#include "server_db.h"
#include "server.h"
#include "common.h"
#include <libpq-fe.h>

static char* 
server_db_load_sql(const char* path, size_t* size)
{
    i32 fd;
    size_t len = 0;
    char* buffer = NULL;

    fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        error("open %s: %s\n", path, ERRSTR);
        return NULL;
    }

    len = fdsize(fd);
    if (len == 0)
    {
        close(fd);
        return NULL;
    }

    buffer = malloc(len + 1);
    *size = len;

    if (read(fd, buffer, len) == -1)
    {
        error("read %s: %s\n", path, ERRSTR);
        free(buffer);
        buffer = NULL;
    }

    buffer[len] = 0x00;

    close(fd);

    return buffer;
}

static bool
db_exec_sql(server_db_t* db, const char* sql)
{
    bool ret = true;
    PGresult* res;

    if (!sql)
    {
        warn("db_exec_sql() sql is NULL!\n");
        return false;
    }

    res = PQexec(db->conn, sql);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        debug("Executing SQL:\n%s\n==================== Done\n", sql);
        error("PQexec() failed: %s\n",
                PQresultErrorMessage(res));
        ret = false;
    }

    PQclear(res);

    return ret;
}

static void
db_notice_processor(UNUSED void* arg, const char* msg)
{
    verbose("db: %s", msg);
}

static bool 
db_exec_schema(server_t* server)
{
    bool ret = true;
    server_db_t db;

    if (!server_db_open(&db, server->conf.database))
        ret = false;

    if (ret && !db_exec_sql(&db, server->db_commands.schema))
        ret = false;

    server_db_close(&db);

    return ret;
}

bool 
server_init_db(server_t* server)
{
    server_db_commands_t* cmd;

    cmd = &server->db_commands;

    cmd->schema = server_db_load_sql(server->conf.sql_schema, &cmd->schema_len);

    cmd->insert_user = server_db_load_sql(server->conf.sql_insert_user, &cmd->insert_user_len);
    cmd->select_user = server_db_load_sql(server->conf.sql_select_user, &cmd->select_user_len);
    cmd->delete_user = server_db_load_sql(server->conf.sql_delete_user, &cmd->delete_user_len);

    cmd->insert_group = server_db_load_sql(server->conf.sql_insert_group, &cmd->insert_group_len);
    cmd->select_group = server_db_load_sql(server->conf.sql_select_group, &cmd->select_group_len);
    cmd->delete_group = server_db_load_sql(server->conf.sql_delete_group, &cmd->delete_group_len);

    cmd->insert_groupmember = server_db_load_sql(server->conf.sql_insert_groupmember, &cmd->insert_groupmember_len);
    cmd->select_groupmember = server_db_load_sql(server->conf.sql_select_groupmember, &cmd->select_groupmember_len);
    cmd->delete_groupmember = server_db_load_sql(server->conf.sql_delete_groupmember, &cmd->delete_groupmember_len);

    cmd->insert_msg = server_db_load_sql(server->conf.sql_insert_msg, &cmd->insert_msg_len);
    cmd->select_msg = server_db_load_sql(server->conf.sql_select_msg, &cmd->select_msg_len);
    cmd->delete_msg = server_db_load_sql(server->conf.sql_delete_msg, &cmd->delete_msg_len);

    cmd->update_user = server_db_load_sql(server->conf.sql_update_user, &cmd->delete_msg_len);
    cmd->insert_userfiles = server_db_load_sql(server->conf.sql_insert_userfiles, &cmd->insert_userfiles_len);

    return db_exec_schema(server);
}

bool
server_db_open(server_db_t* db, const char* dbname)
{
    char user[SYSTEM_USERNAME_LEN];
    char conninfo[DB_CONNINTO_LEN];

    getlogin_r(user, SYSTEM_USERNAME_LEN);

    snprintf(conninfo, DB_CONNINTO_LEN, "dbname=%s user=%s", dbname, user);

    db->conn = PQconnectdb(conninfo);
    if (PQstatus(db->conn) != CONNECTION_OK)
    {
        error("Failed connect to database: %s\n", 
                PQerrorMessage(db->conn));
        return false;
    }

    PQsetNoticeProcessor(db->conn, db_notice_processor, NULL);

    return true;
}

void 
server_db_free(server_t* server)
{
    if (!server)
        return;

    server_db_commands_t* cmd = &server->db_commands;

    free(cmd->schema);

    free(cmd->insert_user);
    free(cmd->select_user);
    free(cmd->delete_user);

    free(cmd->insert_group);
    free(cmd->select_group);
    free(cmd->delete_group);

    free(cmd->insert_groupmember);
    free(cmd->select_groupmember);
    free(cmd->delete_groupmember);

    free(cmd->insert_msg);
    free(cmd->select_msg);
    free(cmd->delete_msg);

    free(cmd->update_user);
    free(cmd->insert_userfiles);
}

void 
server_db_close(server_db_t* db)
{
    if (!db)
        return;

    PQfinish(db->conn);
}

static void
db_row_to_userfile(dbuser_file_t* file, PGresult* res, i32 row)
{
    char* endptr;
    const char* dbhash;
    const char* dbmime_type;
    const char* size_str;
    const char* flags_str;
    const char* ref_count_str;

    dbhash = PQgetvalue(res, row, 0);
    strncpy(file->hash, dbhash, DB_PFP_HASH_MAX);

    size_str = PQgetvalue(res, row, 1);
    file->size = strtoull(size_str, &endptr, 10);
    
    dbmime_type = PQgetvalue(res, row, 2);
    strncpy(file->mime_type, dbmime_type, DB_MIME_TYPE_LEN);

    flags_str = PQgetvalue(res, row, 3);
    file->flags = atoi(flags_str);

    ref_count_str = PQgetvalue(res, row, 4);
    file->ref_count = atoi(ref_count_str);
}

static void
db_row_to_msg(dbmsg_t* msg, PGresult* res, i32 row)
{
    char* endptr;

    const char* msg_id_str = PQgetvalue(res, row, 0);
    msg->msg_id = strtoul(msg_id_str, &endptr, 10);
    
    const char* user_id_str = PQgetvalue(res, row, 1);
    msg->user_id = strtoul(user_id_str, &endptr, 10);

    const char* group_id_str = PQgetvalue(res, row, 2);
    msg->group_id = strtoul(group_id_str, &endptr, 10);

    const char* content = PQgetvalue(res, row, 3);
    strncpy(msg->content, content, DB_MESSAGE_MAX);

    const char* timestamp = PQgetvalue(res, row, 4);
    strncpy(msg->timestamp, timestamp, DB_TIMESTAMP_MAX);
    
    const char* attachments = PQgetvalue(res, row, 5);
    if (attachments)
    {
        msg->attachments = strdup(attachments);
        msg->attachments_inheap = true;
    }
    else
        msg->attachments = "[]";

    const char* parent_msg_id_str = PQgetvalue(res, row, 6);
    if (parent_msg_id_str)
        msg->parent_msg_id = strtoul(parent_msg_id_str, &endptr, 10);
}

static void 
db_row_to_group(dbgroup_t* group, PGresult* res, i32 row)
{
    char* endptr;

    const char* group_id_str = PQgetvalue(res, row, 0);
    if (group_id_str)
        group->group_id = strtoul(group_id_str, &endptr, 10);
    else
        warn("group_id_str is NULL\n");

    const char* owner_id_str = PQgetvalue(res, row, 1);
    if (owner_id_str)
        group->owner_id = strtoul(owner_id_str, &endptr, 10);
    else
        warn("owner_id_str is NULL!\n");

    const char* name = PQgetvalue(res, row, 2);
    if (name)
        strncpy(group->displayname, name, DB_DISPLAYNAME_MAX);
    else
        warn("group displayname is NULL\n");

    const char* desc = PQgetvalue(res, row, 3);
    if (desc)
        strncpy(group->desc, desc, DB_DESC_MAX);
    else
        warn("group desc is NULL!\n");

    const char* flags_str = PQgetvalue(res, row, 4);
    if (flags_str)
        group->flags = strtoul(flags_str, &endptr, 10);
    else
        warn("group flags_str is NULL\n");

    const char* created_at = PQgetvalue(res, row, 5);
    if (created_at)
        strncpy(group->created_at, created_at, DB_TIMESTAMP_MAX);
    else
        warn("group created_at is NULL!\n");
}

static void 
db_row_to_user(dbuser_t* user, PGresult* res, i32 row)
{
    char* endptr;

    const char* id_str = PQgetvalue(res, row, 0);
    if (id_str)
        user->user_id = strtoul(id_str, &endptr, 10);
    else
        warn("id_str is NULL!\n");
    
    const char* username = PQgetvalue(res, row, 1);
    if (username)
        strncpy(user->username, username, DB_USERNAME_MAX);
    else
        warn("username is NULL!\n");

    const char* displayname = PQgetvalue(res, row, 2);
    if (displayname)
        strncpy(user->displayname, displayname, DB_DISPLAYNAME_MAX);
    else
        warn("displayname is NULL!\n");

    const char* bio = PQgetvalue(res, row, 3);
    if (bio)
        strncpy(user->bio, bio, DB_BIO_MAX);

    const void* hash_str = PQgetvalue(res, row, 4);
    size_t hash_size = PQgetlength(res, row, 4);

    hexstr_to_u8(hash_str, hash_size, user->hash);

    const void* salt_str = PQgetvalue(res, row, 5);
    size_t salt_size = PQgetlength(res, row, 5);

    hexstr_to_u8(salt_str, salt_size, user->salt);

    const char* flags_str = PQgetvalue(res, row, 6);
    if (flags_str)
        user->flags = atoi(flags_str);
    else
        warn("flags is NULL!\n");

    const char* created_at = PQgetvalue(res, row, 7);
    if (created_at)
        strncpy(user->created_at, created_at, DB_TIMESTAMP_MAX);
    else
        warn("created_at is NULL!\n");

    const char* pfp_hash = PQgetvalue(res, row, 8);
    if (pfp_hash)
        strncpy(user->pfp_hash, pfp_hash, DB_PFP_HASH_MAX);
}

dbuser_t* 
server_db_get_user(server_db_t* db, u32 user_id, const char* username_to) 
{
    PGresult* res;
    dbuser_t* user = NULL;
    ExecStatusType status_type;
    int rows;

    const char* vals[2];
    char user_id_str[50];
    snprintf(user_id_str, 50, "%d", user_id);
    vals[0] = user_id_str;
    vals[1] = username_to;

    int lens[2] = {
        strlen(user_id_str),
        (username_to) ? strlen(username_to) : 0
    };

    int formats[2] = {0, 0};

    res = PQexecParams(db->conn, db->cmd->select_user, 2, NULL, vals, lens, formats, 0);
    status_type = PQresultStatus(res);
    if (status_type != PGRES_TUPLES_OK)
    {
        error("PQexecParams() failed (%d) on select_user: %s\n",
                    status_type, PQresultErrorMessage(res));
        goto cleanup;
    }

    rows = PQntuples(res);
    if (rows == 0)
    {
        error("No user found %zu or '%s'\n", user_id, username_to);
        goto cleanup;
    }

    user = calloc(1, sizeof(dbuser_t));

    db_row_to_user(user, res, 0);

cleanup:
    PQclear(res);
    return user;
}

dbuser_t* 
server_db_get_user_from_id(server_db_t* db, u32 user_id)
{
    return server_db_get_user(db, user_id, "");
}

dbuser_t* 
server_db_get_user_from_name(server_db_t* db, const char* username)
{
    return server_db_get_user(db, -1, username);
}

dbuser_t* 
server_db_get_users_from_group(UNUSED server_db_t* db, UNUSED u32 group_id, UNUSED u32* n)
{
    debug("Implement server_db_get_users_from_group()!\n");
    return NULL;
}

bool 
server_db_insert_user(server_db_t* db, dbuser_t* user)
{
    bool ret = true;
    PGresult* res;
    const char* vals[5] = {
        user->username,
        user->displayname,
        user->bio,
        (const char*)user->hash,
        (const char*)user->salt
    };
    int formats[5] = {
        0,
        0,
        0,
        1,
        1
    };
    int lens[5] = {
        strnlen(user->username, DB_USERNAME_MAX),
        strnlen(user->displayname, DB_DISPLAYNAME_MAX),
        strnlen(user->bio, DB_BIO_MAX),
        SERVER_HASH_SIZE,
        SERVER_SALT_SIZE
    };

    res = PQexecParams(db->conn, db->cmd->insert_user, 5, NULL, vals, lens, formats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        error("PQexecParams() failed insert_user: %s\n", 
                    PQresultErrorMessage(res));
        ret = false;
    }

    PQclear(res);

    return ret;
}

bool 
server_db_update_user(server_db_t* db, const char* new_username,
                           const char* new_displayname,
                           const char* new_hash_name, const u32 user_id) 
{
    PGresult* res;
    bool ret = true;
    char user_id_str[50];
    snprintf(user_id_str, 50, "%u", user_id);

    const char* vals[7] = {
        (new_username) ? "true" : "false",
        new_username,
        (new_displayname) ? "true" : "false",
        new_displayname,
        (new_hash_name) ? "true" : "false",
        new_hash_name,
        user_id_str
    };
    const int formats[7] = {0};
    const int lens[7] = {
        strlen(vals[0]),
        (new_username) ?    strnlen(new_username, DB_USERNAME_MAX) : 0,
        strlen(vals[2]),
        (new_displayname) ? strnlen(new_displayname, DB_DISPLAYNAME_MAX) : 0,
        strlen(vals[4]),
        (new_hash_name) ?   strnlen(new_hash_name, DB_PFP_HASH_MAX) : 0,
        strlen(user_id_str)
    };

    res = PQexecParams(db->conn, db->cmd->update_user, 7, NULL, vals, lens, formats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        error("PQexecParams() failed for update_user: %s\n", 
                    PQresultErrorMessage(res));
        ret = false;
    }

    PQclear(res);

    return ret;
}

dbuser_t* 
server_db_get_group_members(server_db_t* db, u32 group_id, u32* n_ptr)
{
    PGresult* res;
    dbuser_t* users = NULL;
    i32 rows = 0;

    char group_id_str[50];
    snprintf(group_id_str, 50, "%u", group_id);

    const char* vals[1] = {
        group_id_str
    };
    const int lens[1] = {
        strlen(group_id_str)
    };
    const int formats[1] = {
        0
    };

    res = PQexecParams(db->conn, db->cmd->select_groupmember, 1, NULL, vals, lens, formats, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        error("PQexecParams() failed for select_groupmembers: %s\n",
                    PQresultErrorMessage(res));
        goto cleanup;
    }

    rows = PQntuples(res);
    if (rows == 0)
        goto cleanup;

    users = calloc(rows, sizeof(dbuser_t));
    
    for (i32 i = 0; i < rows; i++)
    {
        dbuser_t* user = users + i;

        db_row_to_user(user, res, i);
    }

cleanup:
    if (n_ptr)
        *n_ptr = rows;
    PQclear(res);
    return users;
}

bool 
server_db_user_in_group(UNUSED server_db_t* db, UNUSED u32 group_id, UNUSED u32 user_id)
{
    debug("Implement or remove server_db_user_in_group()\n");
    return false;
}

bool 
server_db_insert_group_member(server_db_t* db, u32 group_id, u32 user_id)
{
    PGresult* res;
    bool ret = true;

    char user_id_str[50];
    char group_id_str[50];

    snprintf(user_id_str, 50, "%u", user_id);
    snprintf(group_id_str, 50, "%u", group_id);

    const char* vals[2] = {
        user_id_str,
        group_id_str
    };
    const int formats[2] = {
        0,
        0
    };
    const int lens[2] = {
        strlen(user_id_str),
        strlen(group_id_str)
    };

    res = PQexecParams(db->conn, db->cmd->insert_groupmember, 2, NULL, vals, lens, formats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        error("PQexecParams() failed for insert_groupmember: %s\n",
                    PQresultErrorMessage(res));
        ret = false;
    }

    PQclear(res);

    return ret;
}

static dbgroup_t* 
server_db_get_groups(server_db_t* db, u32 user_id, u32 group_id, u32* n_ptr)
{
    PGresult* res;
    i32 rows;
    dbgroup_t* groups = NULL;

    char user_id_str[50];
    char group_id_str[50];

    snprintf(user_id_str, 50, "%d", user_id);
    snprintf(group_id_str, 50, "%d", group_id);

    const char* vals[2] = {
        group_id_str,
        user_id_str
    };
    const int formats[2] = {0, 0};
    const int lens[2] = {
        strlen(group_id_str),
        strlen(user_id_str)
    };

    res = PQexecParams(db->conn, db->cmd->select_group, 2, NULL, vals, lens, formats, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        error("PGexecParams() failed for select_group: %s\n", 
                    PQresultErrorMessage(res));
        goto cleanup;
    }

    rows = PQntuples(res);
    if (rows == 0)
        goto cleanup;

    groups = calloc(rows, sizeof(dbgroup_t));

    for (i32 i = 0; i < rows; i++)
    {
        dbgroup_t* group = groups + i;

        db_row_to_group(group, res, i);
    }

    if (n_ptr)
        *n_ptr = rows;

cleanup:
    PQclear(res);
    return groups;
}

dbgroup_t* 
server_db_get_group(server_db_t* db, u32 group_id)
{
    return server_db_get_groups(db, -1, group_id, NULL);
}

dbgroup_t*  
server_db_get_all_groups(server_db_t* db, u32* n_ptr)
{
    const char* sql = "SELECT * FROM Groups;";
    PGresult* res;
    i32 rows;
    dbgroup_t* groups = NULL;

    res = PQexecParams(db->conn, sql, 0, NULL, NULL, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        error("PGexecParams() failed for get all groups: %s\n", 
                    PQresultErrorMessage(res));
        goto cleanup; 
    }

    rows = PQntuples(res);
    groups = calloc(rows, sizeof(dbgroup_t));

    for (i32 i = 0; i < rows; i++)
    {
        dbgroup_t* group = groups + i;
        
        db_row_to_group(group, res, i);
    }

    if (n_ptr)
        *n_ptr = rows;

cleanup:
    PQclear(res);
    return groups;
}

dbgroup_t* 
server_db_get_user_groups(server_db_t* db, u32 user_id, u32* n_ptr)
{
    return server_db_get_groups(db, user_id, -1, n_ptr);
}

bool 
server_db_insert_group(server_db_t* db, dbgroup_t* group)
{
    PGresult* res;
    bool ret = true;
    ExecStatusType status_type;

    char owner_id_str[50];
    snprintf(owner_id_str, 50, "%u", group->owner_id);
    char flags_id_str[50];
    snprintf(flags_id_str, 50, "%u", group->flags);

    const char* vals[4] = {
        group->displayname,
        group->desc,
        owner_id_str,
        flags_id_str
    };
    const i32 formats[4] = {
        0,
        0,
        0,
        0
    };
    const i32 lens[4] = {
        strlen(vals[0]),
        strlen(vals[1]),
        strlen(vals[2]),
        strlen(vals[3])
    };

    res = PQexecParams(db->conn, db->cmd->insert_group, 4, NULL, vals, lens, formats, 0);
    status_type = PQresultStatus(res);
    if (status_type == PGRES_TUPLES_OK)
    {
        const char* group_id_str = PQgetvalue(res, 0, 0);
        char* endptr;
        group->group_id = strtoul(group_id_str, &endptr, 10);
    }
    else if (status_type != PGRES_COMMAND_OK)
    {
        error("PQexecParamas() failed for insert_group: %s\n",
                PQresultErrorMessage(res));
        ret = false;
    }
    else
        warn("NO PGRES_TUPLES_OK!\n");

    PQclear(res);
    return ret;
}

static dbmsg_t* 
server_db_get_msgs(server_db_t* db, u32 msg_id, u32 group_id, u32 limit, u32 offset, u32* n_ptr)
{
    PGresult* res;
    i32 rows;
    dbmsg_t* msgs = NULL;

    char group_id_str[50];
    char msg_id_str[50];
    char limit_str[50];
    char offset_str[50];
    i32 lens[4];

    lens[0] = snprintf(group_id_str, 50, "%d", group_id);
    lens[1] = snprintf(msg_id_str, 50, "%d", msg_id);
    lens[2] = snprintf(limit_str, 50, "%u", limit);
    lens[3] = snprintf(offset_str, 50, "%u", offset);

    const char* vals[4] = {
        group_id_str,
        msg_id_str,
        limit_str,
        offset_str
    };
    const i32 formats[4] = {0};

    res = PQexecParams(db->conn, db->cmd->select_msg, 4, NULL, vals, lens, formats, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        error("PGexecParams() failed for select_msg: %s\n",
                PQresultErrorMessage(res));
        goto cleanup;
    }

    rows = PQntuples(res);
    msgs = calloc(rows, sizeof(dbmsg_t));

    for (i32 i = 0; i < rows; i++)
    {
        dbmsg_t* msg = msgs + i;

        db_row_to_msg(msg, res, i);
    }

    if (n_ptr)
        *n_ptr = rows;

cleanup:
    PQclear(res);
    return msgs;
}

dbmsg_t* 
server_db_get_msg(server_db_t* db, u32 msg_id)
{
    return server_db_get_msgs(db, msg_id, -1, 1, 0, NULL);
}

dbmsg_t* 
server_db_get_msgs_from_group(server_db_t* db, u32 group_id, u32 limit, u32 offset, u32* n)
{
    return server_db_get_msgs(db, -1, group_id, limit, offset, n);
}

bool 
server_db_insert_msg(server_db_t* db, dbmsg_t* msg)
{
    PGresult* res;
    bool ret = true;
    i32 lens[4];
    ExecStatusType status_type;

    char user_id_str[50];
    char group_id_str[50];

    if (!msg->attachments)
        msg->attachments = "[]";

    lens[0] = snprintf(user_id_str, 50, "%d", msg->user_id);
    lens[1] = snprintf(group_id_str, 50, "%d", msg->group_id);
    lens[2] = strnlen(msg->content, DB_MESSAGE_MAX);
    lens[3] = strlen(msg->attachments);

    const char* vals[4] = {
        user_id_str,
        group_id_str,
        msg->content,
        msg->attachments
    };
    const i32 formats[4] = {0};

    res = PQexecParams(db->conn, db->cmd->insert_msg, 4, NULL, vals, lens, formats, 0);
    status_type = PQresultStatus(res);
    if (status_type == PGRES_TUPLES_OK)
    {
        const char* msg_id_str = PQgetvalue(res, 0, 0);
        char* endptr;
        msg->msg_id = strtoul(msg_id_str, &endptr, 10);
    }
    else if (status_type != PGRES_COMMAND_OK)
    {
        error("PQexecParams() failed for insert_msg: %s\n",
                    PQresultErrorMessage(res));
        ret = false;
    }
    else
    {
        warn("NO PGRES_TUPLES_OK for insert_msg.\n");
        ret = false;
    }

    PQclear(res);
    return ret;
}

dbuser_file_t* 
server_db_select_userfile(server_db_t* db, const char* hash)
{
    PGresult* res;
    i32 rows;
    dbuser_file_t* file = NULL;
    const char* sql = "SELECT * FROM UserFiles WHERE hash = $1::TEXT;";

    const char* vals[1] = {
        hash
    };
    const i32 lens[1] = {
        strnlen(hash, DB_PFP_HASH_MAX)
    };
    const i32 formats[1] = {0};

    res = PQexecParams(db->conn, sql, 1, NULL, vals, lens, formats, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        error("PQexecParams() failed for select_userfile: %s\n",
                    PQresultErrorMessage(res));
        goto cleanup;
    }

    rows = PQntuples(res);

    if (rows == 0)
        goto cleanup;

    file = calloc(1, sizeof(dbuser_file_t));
    db_row_to_userfile(file, res, 0);

cleanup:
    PQclear(res);
    return file;
}

i32
server_db_select_userfile_ref_count(server_db_t* db, const char* hash)
{
    PGresult* res;
    i32 rows;
    i32 ref_count = 0;
    const char* ref_count_str;
    const char* sql = "SELECT ref_count FROM UserFiles WHERE hash = $1::TEXT;";

    const char* vals[1] = {
        hash
    };
    const i32 lens[1] = {
        strnlen(hash, DB_PFP_HASH_MAX)
    };
    const i32 formats[1] = {0};

    res = PQexecParams(db->conn, sql, 1, NULL, vals, lens, formats, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        error("PQexecParams() failed for select_userfile: %s\n",
                    PQresultErrorMessage(res));
        goto cleanup;
    }

    rows = PQntuples(res);

    if (rows == 0)
        goto cleanup;

    ref_count_str = PQgetvalue(res, 0, 0);
    ref_count = atoi(ref_count_str);

cleanup:
    PQclear(res);
    return ref_count;
}

bool        
server_db_insert_userfile(server_db_t* db, dbuser_file_t* file)
{
    PGresult* res;
    bool ret = true;

    i32 lens[3];
    char size_str[50];

    lens[0] = strnlen(file->hash, DB_PFP_HASH_MAX);
    lens[1] = snprintf(size_str, 50, "%zu", file->size);
    lens[2] = strnlen(file->mime_type, DB_MIME_TYPE_LEN);

    const char* vals[3] = {
        file->hash,
        size_str,
        file->mime_type
    };
    const i32 formats[3] = {0};

    res = PQexecParams(db->conn, db->cmd->insert_userfiles, 3, NULL, vals, lens, formats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        error("PQexecParams() failed for insert_userfiles: %s\n", 
                PQresultErrorMessage(res));
        ret = false;
    }

    PQclear(res);
    return ret;
}

bool        
server_db_delete_userfile(server_db_t* db, const char* hash)
{
    PGresult* res;
    bool ret = false;
    const char* sql = "UPDATE UserFiles SET ref_count = ref_count - 1 WHERE hash = $1::TEXT;";

    const char* vals[1] = {
        hash
    };
    const i32 lens[1] = {
        strnlen(hash, DB_PFP_HASH_MAX)
    };
    const i32 formats[1] = {0};

    res = PQexecParams(db->conn, sql, 1, NULL, vals, lens, formats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        error("PQexecParams() failed for delete_userfile: %s\n",
                PQresultErrorMessage(res));
        goto cleanup;
    }

    if (server_db_select_userfile_ref_count(db, hash) <= 0)
        ret = true;

cleanup:
    PQclear(res);
    return ret;
}

void dbmsg_free(dbmsg_t* msg)
{
    if (!msg)
        return;

    if (msg->attachments_inheap && msg->attachments)
        free(msg->attachments);

    free(msg);
}
