#include "chat/db_def.h"
#include "server.h"
#include "server_log.h"
#include "chat/db.h"
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DB_PIPELINE_QUEUE_SIZE 128

static void
db_init_queue(server_db_t* db, size_t size)
{
    plq_t* q = &db->queue;

    q->begin = calloc(size, sizeof(dbcmd_ctx_t));
    q->end = q->begin + size - 1;
    q->read = q->begin;
    q->write = q->begin;
    q->size = size;
    q->count = 0;
}

static char* 
server_db_load_sql(const char* name)
{
    i32 fd;
    size_t len = 0;
    char* buffer = NULL;
    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "backend/sql/%s.sql", name);

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

    if (!server_db_open(&db, &server->conf, (server->conf.retry_db_connect) ? DB_TRY_RECONNECT : DB_DEFAULT))
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

    cmd->schema = server_db_load_sql("schema");

    cmd->insert_user = server_db_load_sql("insert_user");
    cmd->select_user = server_db_load_sql("select_user");

    cmd->select_connected_users = server_db_load_sql("select_connected_users");
    cmd->select_user_json = server_db_load_sql("select_user_json");

    cmd->insert_group = server_db_load_sql("insert_group");
    cmd->select_user_groups = server_db_load_sql("select_user_groups");
    cmd->select_pub_group = server_db_load_sql("select_public_group");

    cmd->insert_groupmember_code = server_db_load_sql("insert_groupmember_code");

    cmd->select_groupmember = server_db_load_sql("select_groupmember");
    cmd->insert_pub_groupmember = server_db_load_sql("join_pub_group");

    cmd->insert_msg = server_db_load_sql("insert_msg");
    cmd->select_msg = server_db_load_sql("select_msg");

    cmd->select_group_msgs_json = server_db_load_sql("select_group_msgs_json");
    cmd->delete_msg = server_db_load_sql("delete_msg");

    cmd->update_user = server_db_load_sql("update_user");

    cmd->insert_userfiles = server_db_load_sql("insert_userfiles");

    cmd->create_group_code = server_db_load_sql("create_group_code");
    cmd->get_group_code = server_db_load_sql("get_group_codes");
    cmd->delete_group_code = server_db_load_sql("delete_group_code");

    cmd->insert_session = server_db_load_sql("insert_session");
    cmd->select_session = server_db_load_sql("select_session");

    cmd->insert_remember_token = server_db_load_sql("insert_remember_token");
    cmd->select_remember_token = server_db_load_sql("select_remember_token");
    cmd->update_remember_token = server_db_load_sql("update_remember_token");

    cmd->insert_login_attempt = server_db_load_sql("insert_login_attempt");

    return db_exec_schema(server);
}

bool
server_db_open(server_db_t* db, UNUSED server_config_t* config, i32 flags)
{
    i32 retries = 1;
    struct passwd* pw;
    char conninfo[DB_CONNINTO_LEN];
    char dbaddress[DB_ADDRESS_LEN] = "";

    const char* user = getenv("DB_USER");
    const char* password = getenvd("DB_PASSWORD", "");
    const char* dbhost = getenv("DB_HOST");
    const char* dbport = getenv("DB_PORT");
    const char* dbname = getenvd("DB_NAME", "chitychat");

    if (dbhost && dbport)
        snprintf(dbaddress, DB_ADDRESS_LEN, "host=%s port=%s", dbhost, dbport);
    else if (dbhost)
        snprintf(dbaddress, DB_ADDRESS_LEN, "host=%s", dbhost);
    else if (dbport)
        snprintf(dbaddress, DB_ADDRESS_LEN, "port=%s", dbport);

    if (user == NULL)
    {
        pw = getpwuid(geteuid());
        if (pw == NULL)
        {
            fatal("getpwuid: %s\n", strerror(errno));
            return false;
        }
        user = pw->pw_name;
    }

    snprintf(conninfo, DB_CONNINTO_LEN, "%s dbname=%s user=%s password=%s", 
        dbaddress, dbname, user, password);

retry:
    db->conn = PQconnectdb(conninfo);
    if (PQstatus(db->conn) != CONNECTION_OK)
    {
        if (flags & DB_TRY_RECONNECT && retries <= 3)
        {
            sleep(2);
            retries++;
            goto retry;
        }

        error("Failed connect to database: %s\n", 
                PQerrorMessage(db->conn));
        return false;
    }

    PQsetNoticeProcessor(db->conn, db_notice_processor, NULL);
    db->flags = flags;

    if (flags & DB_PIPELINE)
    {
        if (PQenterPipelineMode(db->conn) != 1)
        {
            error("Enter pipeline mode: %s\n",
                  PQerrorMessage(db->conn));
            goto err;
        }
        db_init_queue(db, DB_PIPELINE_QUEUE_SIZE);
    }
    if (flags & DB_NONBLOCK && 
        PQsetnonblocking(db->conn, 1) != 0)
    {
        error("Set non-blocking: %s\n",
              PQerrorMessage(db->conn));
        goto err;
    }
    db->fd = PQsocket(db->conn);

    return true;
err:
    server_db_close(db);
    return false;
}

void 
server_db_free(server_t* server)
{
    if (!server)
        return;

    server_db_commands_t* cmd = &server->db_commands;

    free((void*)cmd->schema);

    free((void*)cmd->insert_user);
    free((void*)cmd->select_user);
    free((void*)cmd->select_user_json);
    free((void*)cmd->select_connected_users);
    free((void*)cmd->delete_user);

    free((void*)cmd->insert_group);
    free((void*)cmd->select_user_groups);
    free((void*)cmd->select_pub_group);
    free((void*)cmd->delete_group);

    free((void*)cmd->insert_groupmember_code);
    free((void*)cmd->insert_pub_groupmember);
    free((void*)cmd->select_groupmember);
    free((void*)cmd->delete_groupmember);

    free((void*)cmd->insert_msg);
    free((void*)cmd->select_msg);
    free((void*)cmd->select_group_msgs_json);
    free((void*)cmd->delete_msg);

    free((void*)cmd->update_user);
    free((void*)cmd->insert_userfiles);

    free((void*)cmd->create_group_code);
    free((void*)cmd->get_group_code);
    free((void*)cmd->delete_group_code);

    free((void*)cmd->insert_session);
    free((void*)cmd->select_session);
}

void 
server_db_close(server_db_t* db)
{
    if (!db)
        return;

    if (db->flags & DB_PIPELINE)
        free(db->queue.begin);
    PQfinish(db->conn);
}

void 
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
        strncpy(group->displayname, name, DB_DISPLAYNAME_MAX - 1);
    else
        warn("group displayname is NULL\n");

    const char* desc = PQgetvalue(res, row, 3);
    if (desc)
        strncpy(group->desc, desc, DB_DESC_MAX - 1);
    else
        warn("group desc is NULL!\n");

    const char* public_str = PQgetvalue(res, row, 4);
    if (public_str && *public_str == 't')
        group->public = true;
    else
        group->public = false;

    const char* created_at = PQgetvalue(res, row, 5);
    if (created_at)
        strncpy(group->created_at, created_at, DB_TIMESTAMP_MAX - 1);
    else
        warn("group created_at is NULL!\n");
}

void 
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
        strncpy(user->username, username, DB_USERNAME_MAX - 1);
    else
        warn("username is NULL!\n");

    const char* displayname = PQgetvalue(res, row, 2);
    if (displayname)
        strncpy(user->displayname, displayname, DB_DISPLAYNAME_MAX - 1);
    else
        warn("displayname is NULL!\n");

    const char* bio = PQgetvalue(res, row, 3);
    if (bio)
        strncpy(user->bio, bio, DB_BIO_MAX - 1);

    const void* hash_str = PQgetvalue(res, row, 4);
    size_t hash_size = PQgetlength(res, row, 4);

    hexstr_to_u8(hash_str, hash_size, user->hash);

    const void* salt_str = PQgetvalue(res, row, 5);
    size_t salt_size = PQgetlength(res, row, 5);

    hexstr_to_u8(salt_str, salt_size, user->salt);

    // const char* flags_str = PQgetvalue(res, row, 6);
    // if (flags_str)
    //     user->flags = atoi(flags_str);
    // else
    //     warn("flags is NULL!\n");

    const char* created_at = PQgetvalue(res, row, 6);
    if (created_at)
        strncpy(user->created_at, created_at, DB_TIMESTAMP_MAX - 1);
    else
        warn("created_at is NULL!\n");

    const char* pfp_hash = PQgetvalue(res, row, 7);
    if (pfp_hash)
        strncpy(user->pfp_hash, pfp_hash, DB_PFP_HASH_MAX - 1);
}
