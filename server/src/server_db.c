#include "server_db.h"
#include "server.h"
#include "common.h"

bool server_db_open(server_t* server)
{
    i32 ret = sqlite3_open(server->conf.database, &server->db.db);
    if (ret)
    {
        fatal("Opening database (%s) failed: %s\n", server->conf.database, sqlite3_errmsg(server->db.db));
        return false;
    }

    return true;
}

void server_db_close(server_t* server)
{
    if (!server)
        return;

    sqlite3_close(server->db.db);
}

dbuser_t* server_db_get_user_from_id(server_t* server, u64 user_id)
{
    return NULL;
}

dbuser_t* server_db_get_user_from_name(server_t* server, const char* username)
{
    return NULL;
}

dbuser_t* server_db_get_users_from_group(server_t* server, u64 group_id, u32* n)
{
    return NULL;
}

bool server_db_insert_user(server_t* server, dbuser_t* user)
{
    return false;
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
    return false;
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