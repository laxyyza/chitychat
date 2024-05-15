#include "chat/rtusm.h"
#include "chat/db_def.h"
#include "chat/user.h"
#include "chat/db_user.h"
#include "chat/db.h"
#include "server_websocket.h"

const char* const rtusm_status_str[RTUSM_STATUS_LEN] = {
    "offline",
    "online",
    "away",
    "dnd"
};

static const char*
do_rtusm_broadcast(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    u32 user_id;
    size_t size;
    u32* user_ids;
    rtusm_t* status;
    rtusm_new_t new;
    json_object* json;
    const char* status_str;
    const char* pfp_hash = ctx->param.rtusm.pfp_hash;

    if (ctx->ret == DB_ASYNC_ERROR || ctx->data_size == 0)
    {
        free((void*)pfp_hash);
        return NULL;
    }

    user_ids = ctx->data;
    size = ctx->data_size;
    status = &ctx->param.rtusm.status;
    new = ctx->param.rtusm.new;
    user_id = ctx->param.rtusm.user_id;
    status_str = rtusm_status_str[status->status];

    json = json_object_new_object();
    json_object_object_add(json, "cmd",
                           json_object_new_string("rtusm"));
    json_object_object_add(json, "user_id",
                           json_object_new_int(user_id));
    if (new.status)
    {
        json_object_object_add(json, "status",
                               json_object_new_string(status_str));
    }

    if (new.typing && status->typing_group_id)
    {
        json_object_object_add(json, "typing",
                               json_object_new_boolean(status->typing));
        json_object_object_add(json, "typing_group_id",
                               json_object_new_int(status->typing_group_id));
    }

    if (new.pfp)
    {
        json_object_object_add(json, "pfp_name",
                               json_object_new_string(pfp_hash));
    }

    for (size_t i = 0; i < size; i++)
    {
        client_t* connected_client = server_get_client_user_id(ew->server, user_ids[i]);
        if (connected_client)
            ws_json_send(connected_client, json);
    }

    json_object_put(json);
    free((void*)pfp_hash);
    return NULL;
}

static void 
rtusm_broadcast(eworker_t* ew, dbuser_t* user, rtusm_new_t new)
{
    const char* pfp_hash = (new.pfp) ? strndup(user->pfp_hash, DB_PFP_HASH_MAX) : NULL;
    dbcmd_ctx_t ctx = {
        .exec = do_rtusm_broadcast,
        .param.rtusm.new = new,
        .param.rtusm.user_id = user->user_id,
        .param.rtusm.status = user->rtusm,
        .param.rtusm.pfp_hash = pfp_hash
    };
    db_async_get_connected_users(&ew->db, user->user_id, &ctx);

    // json_object* packet;
    // dbuser_t* connected_users;
    // u32     n_users;
    // client_t* connected_client;
    // const rtusm_t* user_status = &user->rtusm;
    // const char* const status_str = rtusm_status_str[user_status->status];
    //
    // packet = json_object_new_object();
    //
    // json_object_object_add(packet, "cmd", 
    //                        json_object_new_string("rtusm"));
    // json_object_object_add(packet, "user_id",
    //                        json_object_new_int(user->user_id));
    // if (new.status)
    // {
    //     json_object_object_add(packet, "status",
    //                            json_object_new_string(status_str));
    // }
    //
    // if (new.typing && user_status->typing_group_id)
    // {
    //     json_object_object_add(packet, "typing",
    //                            json_object_new_boolean(user_status->typing));
    //     json_object_object_add(packet, "typing_group_id",
    //                            json_object_new_int(user_status->typing_group_id));
    // }
    //
    // if (new.pfp)
    // {
    //     json_object_object_add(packet, "pfp_name",
    //                            json_object_new_string(user->pfp_hash));
    // }
    // 
    // connected_users = server_db_get_connected_users(&ew->db, user->user_id, &n_users);
    //
    // for (u32 i = 0; i < n_users; i++)
    // {
    //     connected_client = server_get_client_user_id(ew->server, connected_users[i].user_id); 
    //     if (connected_client)
    //         ws_json_send(connected_client, packet);
    // }
    //
    // json_object_put(packet);
    // free(connected_users);
}

void    
server_rtusm_set_user_status(eworker_t* ew, dbuser_t* user, enum rtusm_status status)
{
    if (!ew || !user)
        return;

    user->rtusm.status = status;
}

void    
server_rtusm_user_disconnect(eworker_t* ew, dbuser_t* user)
{
    user->rtusm.status = USER_OFFLINE;
    user->rtusm.typing_group_id = 0;
    user->rtusm.typing = 0;

    const rtusm_new_t new = {
        .status = 1,
        .typing = 0,
        .pfp    = 0 
    };

    rtusm_broadcast(ew, user, new);
}

void    
server_rtusm_user_connect(eworker_t* ew, dbuser_t* user)
{
    user->rtusm.status = USER_ONLINE;

    const rtusm_new_t new = {
        .status = 1,
        .typing = 0,
        .pfp    = 0 
    };

    rtusm_broadcast(ew, user, new);
}

const char* 
rtusm_get_status_str(enum rtusm_status status)
{
    return rtusm_status_str[status];
}

void    
server_rtusm_user_pfp_change(eworker_t* ew, dbuser_t* user)
{
    const rtusm_new_t new = {
        .status = 0,
        .typing = 0,
        .pfp    = 1
    };

    rtusm_broadcast(ew, user, new);
}
