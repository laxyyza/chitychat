#include "server_db.h"

bool        server_db_init(server_t* server)
{
    return true;
}

dbuser_t*   server_db_get_user_from_id(server_t* server, u64 user_id)
{
    return NULL;
}

dbuser_t*   server_db_get_user_from_name(server_t* server, const char* username)
{
    return NULL;
}

dbuser_t*   server_db_get_users_from_group(server_t* server, u64 group_id, u32* n)
{
    return NULL;
}

bool        server_db_insert_user(server_t* server, dbuser_t* user)
{
    return false;
}

dbgroup_t*  server_db_get_group(server_t* server, u64 group_id)
{
    return NULL;
}

dbgroup_t*  server_db_get_user_groups(server_t* server, u64 user_id, u32* n)
{
    return NULL;
}

bool        server_db_insert_group(server_t* server, dbgroup_t* group)
{
    return false;
}

dbmsg_t*    server_db_get_msg(server_t* server, u64 msg_id)
{
    return NULL;
}

dbmsg_t*    server_db_get_msgs_from_group(server_t* server, u64 group_id, u32 max, u32* n)
{
    return NULL;
}

bool        server_db_insert_msg(server_t* server, dbmsg_t* msg)
{
    return false;
}