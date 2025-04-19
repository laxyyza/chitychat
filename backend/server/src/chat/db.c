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

static bool
server_db_load_sql(sql_query_t* q, const char* name)
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
        return false;
    }

    len = fdsize(fd);
    if (len == 0)
    {
        close(fd);
        return false;
    }

    buffer = malloc(len + 1);

    if (read(fd, buffer, len) == -1)
    {
        error("read %s: %s\n", path, ERRSTR);
        free(buffer);
        return false;
    }

    buffer[len] = 0x00;

    close(fd);

    q->sql = buffer;
    q->filename = name;

    return true;
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

    if (ret && !db_exec_sql(&db, server->db_commands.schema.sql))
        ret = false;

    server_db_close(&db);

    return ret;
}

bool 
server_init_db(server_t* server)
{
    server_db_commands_t* cmd;

    cmd = &server->db_commands;

    if (!server_db_load_sql(&cmd->schema, "schema"))
        return false;

    if (!server_db_load_sql(&cmd->insert_user, "insert_user"))
        return false;
    if (!server_db_load_sql(&cmd->select_user, "select_user"))
        return false;

    if (!server_db_load_sql(&cmd->select_connected_users, "select_connected_users"))
        return false;

    if (!server_db_load_sql(&cmd->select_user_json, "select_user_json"))
        return false;

    if (!server_db_load_sql(&cmd->update_user, "update_user"))
        return false;

    if (!server_db_load_sql(&cmd->insert_userfiles, "insert_userfiles"))
        return false;

    if (!server_db_load_sql(&cmd->insert_remember_token, "insert_remember_token"))
        return false;
    if (!server_db_load_sql(&cmd->select_remember_token, "select_remember_token"))
        return false;
    if (!server_db_load_sql(&cmd->update_remember_token, "update_remember_token"))
        return false;

    if (!server_db_load_sql(&cmd->insert_login_attempt, "insert_login_attempt"))
        return false;

    if (!server_db_load_sql(&cmd->insert_remember_token_usage, "insert_remember_token_usage"))
        return false;

    return db_exec_schema(server);
}

bool
server_db_open(server_db_t* db, server_config_t* config, i32 flags)
{
    i32 retries = 1;
    struct passwd* pw;
    char conninfo[DB_CONNINTO_LEN];
    char dbaddress[DB_ADDRESS_LEN] = "";

    if (config->sql_time)
        flags |= DB_SHOW_TIME;

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

    free((void*)cmd->schema.sql);

    free((void*)cmd->insert_user.sql);
    free((void*)cmd->select_user.sql);
    free((void*)cmd->select_user_json.sql);
    free((void*)cmd->select_connected_users.sql);
    free((void*)cmd->delete_user.sql);

    free((void*)cmd->update_user.sql);
    free((void*)cmd->insert_userfiles.sql);

    free((void*)cmd->insert_session.sql);
    free((void*)cmd->select_session.sql);


    free((void*)cmd->select_remember_token.sql);
    free((void*)cmd->insert_remember_token.sql);
    free((void*)cmd->insert_remember_token_usage.sql);
    free((void*)cmd->insert_login_attempt.sql);
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
