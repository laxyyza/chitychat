#include "server_db.h"
#include "server.h"
#include "common.h"
#include <fcntl.h>

static char* server_db_load_sql(const char* path, size_t* size)
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

static i32 db_exec_sql(server_t* server, const char* sql, void* callback)
{
    i32 ret;

    if (!sql)
    {
        warn("db_exec_sql() sql is NULL!\n");
        return -1;
    }

    char* errmsg;
    ret = sqlite3_exec(server->db.db, sql, callback, server, &errmsg);
    if (ret != SQLITE_OK)
    {
        debug("Executing SQL:\n%s\n==================== Done\n", sql);
        error("sqlite3_exec() failed: %s\n", errmsg);
        sqlite3_free(errmsg);
    }

    return ret;
}

bool server_db_open(server_t* server)
{
    i32 ret = sqlite3_open(server->conf.database, &server->db.db);
    if (ret)
    {
        fatal("Opening database (%s) failed: %s\n", server->conf.database, sqlite3_errmsg(server->db.db));
        return false;
    }
    server_db_t* db = &server->db;

    db->schema = server_db_load_sql(server->conf.sql_schema, &db->schema_len);

    db->insert_user = server_db_load_sql(server->conf.sql_insert_user, &db->insert_user_len);
    db->select_user = server_db_load_sql(server->conf.sql_select_user, &db->select_user_len);
    db->delete_user = server_db_load_sql(server->conf.sql_delete_user, &db->delete_user_len);

    db->insert_group = server_db_load_sql(server->conf.sql_insert_group, &db->insert_group_len);
    db->select_group = server_db_load_sql(server->conf.sql_select_group, &db->select_group_len);
    db->delete_group = server_db_load_sql(server->conf.sql_delete_group, &db->delete_group_len);

    db->insert_groupmember = server_db_load_sql(server->conf.sql_insert_groupmember, &db->insert_groupmember_len);
    db->select_groupmember = server_db_load_sql(server->conf.sql_select_groupmember, &db->select_groupmember_len);
    db->delete_groupmember = server_db_load_sql(server->conf.sql_delete_groupmember, &db->delete_groupmember_len);

    db->insert_msg = server_db_load_sql(server->conf.sql_insert_msg, &db->insert_msg_len);
    db->select_msg = server_db_load_sql(server->conf.sql_select_msg, &db->select_msg_len);
    db->delete_msg = server_db_load_sql(server->conf.sql_delete_msg, &db->delete_msg_len);

    db->update_user = server_db_load_sql(server->conf.sql_update_user, &db->delete_msg_len);

    db_exec_sql(server, db->schema, NULL);

    return true;
}

void server_db_close(server_t* server)
{
    if (!server)
        return;

    server_db_t* db = &server->db;

    free(db->schema);

    free(db->insert_user);
    free(db->select_user);
    free(db->delete_user);

    free(db->insert_group);
    free(db->select_group);
    free(db->delete_group);

    free(db->insert_groupmember);
    free(db->select_groupmember);
    free(db->delete_groupmember);

    free(db->insert_msg);
    free(db->select_msg);
    free(db->delete_msg);
    
    sqlite3_close(server->db.db);
}

dbuser_t* server_db_get_user(server_t* server, u64 user_id, const char* username_to) 
{
    dbuser_t* user = calloc(1, sizeof(dbuser_t));

    sqlite3_stmt* stmt;
    i32 ret = sqlite3_prepare_v2(server->db.db, server->db.select_user, -1, &stmt, NULL);

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, username_to, -1, SQLITE_TRANSIENT);
    bool found = false;

    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        user->user_id = sqlite3_column_int(stmt, 0);

        const char* username = (const char*)sqlite3_column_text(stmt, 1);
        if (username)
            strncpy(user->username, username, DB_USERNAME_MAX);
        else
            warn("user %u username is NULL!\n", user->user_id);

        const char* displayname = (const char*)sqlite3_column_text(stmt, 2);
        if (displayname)
            strncpy(user->displayname, displayname, DB_DISPLAYNAME_MAX);
        else
            warn("user %u displayname is NULL\n", user->user_id);

        const char* bio = (const char*)sqlite3_column_text(stmt, 3);
        if (bio)
            strncpy(user->bio, bio, DB_BIO_MAX);

        const u8* hash = sqlite3_column_blob(stmt, 4);
        if (hash)
            memcpy(user->hash, hash, SERVER_HASH_SIZE);
        else
            warn("user %u hash is NULL\n", user->user_id);

        const u8* salt = sqlite3_column_blob(stmt, 5);
        if (salt)
            memcpy(user->salt, salt, SERVER_SALT_SIZE);
        else
            warn("user %u salt is NULL!\n", user->user_id);

        const char* created_at = (const char*)sqlite3_column_text(stmt, 6);
        if (created_at)
            strncpy(user->created_at, created_at, DB_USERNAME_MAX);
        else
            warn("user %u created_at is NULL!\n", user->user_id);

        const char* pfp_name = (const char*)sqlite3_column_text(stmt, 7);
        if (pfp_name)
            strncpy(user->pfp_name, pfp_name, DB_PFP_NAME_MAX);
        else
            warn("user %u pfp_name is NULL\n", user->user_id);

        found = true;
        break;
        // info("ID: %zu, username: '%s', displayname: '%s'\n", user->user_id, user->username, user->displayname);
    }

    sqlite3_finalize(stmt);

    if (!found)
    {
        free(user);
        user = NULL;
    }

    return user;
}

dbuser_t* server_db_get_user_from_id(server_t* server, u64 user_id)
{
    return server_db_get_user(server, user_id, "");
}

dbuser_t* server_db_get_user_from_name(server_t* server, const char* username)
{
    return server_db_get_user(server, -1, username);
}

dbuser_t* server_db_get_users_from_group(server_t* server, u64 group_id, u32* n)
{
    return NULL;
}

bool server_db_insert_user(server_t* server, dbuser_t* user)
{
    sqlite3_stmt* stmt;
    i32 rc = sqlite3_prepare_v2(server->db.db, server->db.insert_user, -1, &stmt, NULL);
    bool ret = true;

    sqlite3_bind_text(stmt, 1, user->username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user->displayname, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user->bio, -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 4, user->hash, SERVER_HASH_SIZE, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 5, user->salt, SERVER_SALT_SIZE, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        error("sqlite3_step: %s\n", sqlite3_errmsg(server->db.db));
        ret = false;
    }

    sqlite3_finalize(stmt);

    return ret;
}

bool server_db_update_user(server_t* server, const char* new_username,
                           const char* new_displayname,
                           const char* new_pfp_name, const u64 user_id) 
{
    sqlite3_stmt* stmt;
    i32 rc = sqlite3_prepare_v2(server->db.db, server->db.update_user, -1, &stmt, NULL);
    bool ret = true;

    const i32 username_con = (new_username != NULL);
    const i32 displayname_con = (new_displayname != NULL);
    const i32 pfp_name_con = (new_pfp_name != NULL);

    sqlite3_bind_int(stmt, 1, username_con);
    sqlite3_bind_text(stmt, 2, new_username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, displayname_con);
    sqlite3_bind_text(stmt, 4, new_displayname, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, pfp_name_con);
    sqlite3_bind_text(stmt, 6, new_pfp_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, user_id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        error("sqlite3_step update user: %s\n", sqlite3_errmsg(server->db.db));
        ret = false;
    }

    sqlite3_finalize(stmt);

    return ret;
}

dbuser_t* server_db_get_group_members(server_t* server, u64 group_id, u32* n_ptr)
{
    sqlite3_stmt* stmt;
    i32 rc = sqlite3_prepare_v2(server->db.db, server->db.select_groupmember, -1, &stmt, NULL);
    dbuser_t* users = malloc(sizeof(dbuser_t));
    u32 n = 0;

    sqlite3_bind_int(stmt, 1, group_id);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        n++;
        if (n > 1)
            users = realloc(users, sizeof(dbuser_t) * n);
        dbuser_t* user = users + (n - 1);
        memset(user, 0, sizeof(dbuser_t));

        user->user_id = sqlite3_column_int(stmt, 0);
        const char* username = (const char*)sqlite3_column_text(stmt, 1);
        strncpy(user->username, username, DB_USERNAME_MAX);
        const char* displayname = (const char*)sqlite3_column_text(stmt, 2);
        strncpy(user->displayname, displayname, DB_DISPLAYNAME_MAX);
        const char* bio = (const char*)sqlite3_column_text(stmt, 3);
        if (bio)
            strncpy(user->bio, bio, DB_BIO_MAX);
    }
    if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        error("Failed while getting group members: %s\n", sqlite3_errmsg(server->db.db));
    }

    sqlite3_finalize(stmt);
    if (n == 0)
    {
        free(users);
        users = NULL;
    }
    *n_ptr = n;
    return users;
}

bool server_db_user_in_group(server_t* server, u64 group_id, u64 user_id)
{
    return false;
}

bool server_db_insert_group_member(server_t* server, u64 group_id, u64 user_id)
{
    sqlite3_stmt* stmt;
    i32 rc = sqlite3_prepare_v2(server->db.db, server->db.insert_groupmember, -1, &stmt, NULL);
    bool ret = true;

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, group_id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        error("Failed to insert group member: %s\n", sqlite3_errmsg(server->db.db));
        ret = false;
    }

    sqlite3_finalize(stmt);

    return ret;
}

static dbgroup_t* server_db_get_groups(server_t* server, u64 user_id, u64 group_id, u32* n_ptr)
{
    u32 n = 0;
    u32 i = 0;
    dbgroup_t* groups = malloc(sizeof(dbgroup_t));
    sqlite3_stmt* stmt;
    i32 rc = sqlite3_prepare_v2(server->db.db, server->db.select_group, -1, &stmt, NULL);

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, user_id);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        n++;
        if (n > 1)
            groups = realloc(groups, sizeof(dbgroup_t) * n);

        dbgroup_t* group = groups + (n - 1);
        memset(group, 0, sizeof(dbgroup_t));

        group->group_id = sqlite3_column_int(stmt, 0);
        group->owner_id = sqlite3_column_int(stmt, 1);
        const char* name = (const char*)sqlite3_column_text(stmt, 2);
        strncpy(group->displayname, name, DB_DISPLAYNAME_MAX);
        const char* desc = (const char*)sqlite3_column_text(stmt, 3);
        strncpy(group->desc, desc, DB_DESC_MAX);

        if (!n_ptr)
            break;
    }
    if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        error("Failed while getting groups: %s\n", sqlite3_errmsg(server->db.db));
    }

    sqlite3_finalize(stmt);

    if (n == 0)
    {
        free(groups);
        groups = NULL;
    }
    if (n_ptr)
        *n_ptr = n;

    return groups;
}

dbgroup_t* server_db_get_group(server_t* server, u64 group_id)
{
    return server_db_get_groups(server, -1, group_id, NULL);
}

dbgroup_t*  server_db_get_all_groups(server_t* server, u32* n_ptr)
{
    sqlite3_stmt* stmt;
    const char* sql = "SELECT * FROM Groups;";
    i32 rc = sqlite3_prepare_v2(server->db.db, sql, -1, &stmt, NULL);
    u32 n = 0;
    dbgroup_t* groups = malloc(sizeof(dbgroup_t));

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        n++;
        if (n > 1)
            groups = realloc(groups, sizeof(dbgroup_t) * n);

        dbgroup_t* group = groups + (n - 1);
        memset(group, 0, sizeof(dbgroup_t));

        group->group_id = sqlite3_column_int(stmt, 0);
        group->owner_id = sqlite3_column_int(stmt, 1);
        const char* name = (const char*)sqlite3_column_text(stmt, 2);
        strncpy(group->displayname, name, DB_DISPLAYNAME_MAX);
        const char* desc = (const char*)sqlite3_column_text(stmt, 3);
        strncpy(group->desc, desc, DB_DESC_MAX);
    }
    if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        error("Failed while getting groups: %s\n", sqlite3_errmsg(server->db.db));
    }

    sqlite3_finalize(stmt);

    if (n == 0)
    {
        free(groups);
        groups = NULL;
    }
    *n_ptr = n;

    return groups;
}

dbgroup_t* server_db_get_user_groups(server_t* server, u64 user_id, u32* n_ptr)
{
    return server_db_get_groups(server, user_id, -1, n_ptr);
}

bool server_db_insert_group(server_t* server, dbgroup_t* group)
{
    sqlite3_stmt* stmt;
    i32 rc = sqlite3_prepare_v2(server->db.db, server->db.insert_group, -1, &stmt, NULL);
    bool ret = true;

    sqlite3_bind_text(stmt, 1, group->displayname, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, group->desc, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, group->owner_id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        error("Failed to insert group: %s\n", sqlite3_errmsg(server->db.db));
        ret = false;
    }
    sqlite3_int64 rowid = sqlite3_last_insert_rowid(server->db.db);
    group->group_id = rowid;

    sqlite3_finalize(stmt);

    return ret;
}

static dbmsg_t* server_db_get_msgs(server_t* server, u64 msg_id, u64 group_id, u32* n_ptr)
{
    sqlite3_stmt* stmt;
    i32 rc = sqlite3_prepare_v2(server->db.db, server->db.select_msg, -1, &stmt, NULL);
    dbmsg_t* msgs = malloc(sizeof(dbmsg_t));
    u32 n = 0;

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, msg_id);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        n++;
        if (n > 1)
            msgs = realloc(msgs, sizeof(dbmsg_t) * n);

        dbmsg_t* msg = msgs + (n - 1);
        memset(msg, 0, sizeof(dbmsg_t));

        msg->msg_id = sqlite3_column_int(stmt, 0);
        msg->user_id = sqlite3_column_int(stmt, 1);
        msg->group_id = sqlite3_column_int(stmt, 2);
        const char* content = (const char*)sqlite3_column_text(stmt, 3);
        strncpy(msg->content, content, DB_MESSAGE_MAX);
        const char* timestamp = (const char*)sqlite3_column_text(stmt, 4);
        strncpy(msg->timestamp, timestamp, DB_TIMESTAMP_MAX);

        if (!n_ptr)
            break;
    }
    sqlite3_finalize(stmt);

    if (n == 0)
    {
        free(msgs);
        msgs = NULL;
    }
    if (n_ptr)
        *n_ptr = n;
    return msgs;
}

dbmsg_t* server_db_get_msg(server_t* server, u64 msg_id)
{
    return server_db_get_msgs(server, msg_id, -1, NULL);
}

dbmsg_t* server_db_get_msgs_from_group(server_t* server, u64 group_id, u32 max, u32* n)
{
    return server_db_get_msgs(server, -1, group_id, n);
}

bool server_db_insert_msg(server_t* server, dbmsg_t* msg)
{
    sqlite3_stmt* stmt;
    i32 rc = sqlite3_prepare_v2(server->db.db, server->db.insert_msg, -1, &stmt, NULL);
    bool ret = true;

    sqlite3_bind_int(stmt, 1, msg->user_id);
    sqlite3_bind_int(stmt, 2, msg->group_id);
    sqlite3_bind_text(stmt, 3, msg->content, -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        error("Failed to insert msg: %s\n", sqlite3_errmsg(server->db.db));
        ret = false;
    }
    sqlite3_int64 rowid = sqlite3_last_insert_rowid(server->db.db);
    msg->msg_id = rowid;

    sqlite3_finalize(stmt);

    return ret;
}