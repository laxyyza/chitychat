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

    debug("Executing SQL:\n%s\n==================== Done\n", sql);

    char* errmsg;
    ret = sqlite3_exec(server->db.db, sql, callback, server, &errmsg);
    if (ret != SQLITE_OK)
    {
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
        strncpy(user->username, username, DB_USERNAME_MAX);
        const char* displayname = (const char*)sqlite3_column_text(stmt, 2);
        strncpy(user->displayname, displayname, DB_DISPLAYNAME_MAX);
        const char* bio = (const char*)sqlite3_column_text(stmt, 3);
        if (bio)
            strncpy(user->bio, bio, DB_BIO_MAX);
        const char* password = (const char*)sqlite3_column_text(stmt, 4);
        strncpy(user->password, password, DB_PASSWORD_MAX);

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
    sqlite3_bind_text(stmt, 4, user->password, -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        error("sqlite3_step: %s\n", sqlite3_errmsg(server->db.db));
        ret = false;
    }

    sqlite3_finalize(stmt);

    return ret;
}

dbgroup_t* server_db_get_group(server_t* server, u64 group_id)
{
    return NULL;
}

dbgroup_t* server_db_get_user_groups(server_t* server, u64 user_id, u32* n)
{
    return NULL;
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

dbmsg_t* server_db_get_msg(server_t* server, u64 msg_id)
{
    return NULL;
}

dbmsg_t* server_db_get_msgs_from_group(server_t* server, u64 group_id, u32 max, u32* n)
{
    return NULL;
}

bool server_db_insert_msg(server_t* server, dbmsg_t* msg)
{
    return false;
}