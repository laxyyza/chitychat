#include "chat/rtusm.h"
#include "chat/user.h"
#include "chat/db.h"
#include "server_websocket.h"

typedef struct 
{
    bool    status:1;
    bool    typing:1;
    bool    pfp:1;
} rtusm_new_t;

const char* const rtusm_status_str[RTUSM_STATUS_LEN] = {
    "offline",
    "online",
    "away",
    "dnd"
};

static void 
rtusm_broadcast(server_thread_t* th, dbuser_t* user, rtusm_new_t new)
{
    json_object* packet;
    dbuser_t* connected_users;
    u32     n_users;
    const client_t* connected_client;
    const rtusm_t* user_status = &user->rtusm;
    const char* const status_str = rtusm_status_str[user_status->status];

    packet = json_object_new_object();

    json_object_object_add(packet, "cmd", 
                           json_object_new_string("rtusm"));
    json_object_object_add(packet, "user_id",
                           json_object_new_int(user->user_id));
    if (new.status)
    {
        json_object_object_add(packet, "status",
                               json_object_new_string(status_str));
    }

    if (new.typing && user_status->typing_group_id)
    {
        json_object_object_add(packet, "typing",
                               json_object_new_boolean(user_status->typing));
        json_object_object_add(packet, "typing_group_id",
                               json_object_new_int(user_status->typing_group_id));
    }

    if (new.pfp)
    {
        json_object_object_add(packet, "pfp_name",
                               json_object_new_string(user->pfp_hash));
    }
    
    connected_users = server_db_get_connected_users(&th->db, user->user_id, &n_users);

    for (u32 i = 0; i < n_users; i++)
    {
        connected_client = server_get_client_user_id(th->server, connected_users[i].user_id); 
        if (connected_client)
            ws_json_send(connected_client, packet);
    }

    json_object_put(packet);
    free(connected_users);
}

void    
server_rtusm_set_user_status(server_thread_t* th, dbuser_t* user, enum rtusm_status status)
{
    if (!th || !user)
        return;

    user->rtusm.status = status;
}

void    
server_rtusm_user_disconnect(server_thread_t* th, dbuser_t* user)
{
    user->rtusm.status = USER_OFFLINE;
    user->rtusm.typing_group_id = 0;
    user->rtusm.typing = 0;

    const rtusm_new_t new = {
        .status = true,
        .typing = false,
        .pfp = false
    };

    rtusm_broadcast(th, user, new);
}

void    
server_rtusm_user_connect(server_thread_t* th, dbuser_t* user)
{
    user->rtusm.status = USER_ONLINE;

    const rtusm_new_t new = {
        .status = true,
        .typing = false,
        .pfp = false
    };

    rtusm_broadcast(th, user, new);
}

const char* 
rtusm_get_status_str(enum rtusm_status status)
{
    return rtusm_status_str[status];
}

void    
server_rtusm_user_pfp_change(server_thread_t* th, dbuser_t* user)
{
    const rtusm_new_t new = {
        .status = false,
        .typing = false,
        .pfp = true
    };

    rtusm_broadcast(th, user, new);
}
