#include "chat/group.h"
#include "chat/db.h"
#include "chat/db_def.h"
#include "chat/db_group.h"
#include "chat/ws_text_frame.h"
#include "json_object.h"
#include "server_websocket.h"

static const char*
do_group_broadcast(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    json_object* json = ctx->param.json;
    const i32* member_ids = ctx->data;
    client_t* member_client;
    size_t n_members = ctx->data_size;

    for (size_t i = 0; i < n_members; i++)
    {
        member_client = server_get_client_user_id(ew->server, member_ids[i]);
        if (member_client)
            ws_json_send(member_client, json);
    }

    json_object_put(json);

    return NULL;
}

static void 
server_group_broadcast(eworker_t* ew, u32 group_id, json_object* json)
{
    dbcmd_ctx_t ctx = {
        .exec = do_group_broadcast,
        .client = NULL,
        .flags = DB_CTX_NO_JSON,
        .param.json = json
    };

    if (!db_async_get_group_member_ids(&ew->db, group_id, &ctx))
        json_object_put(json);
}

static void 
server_delete_attachments_json(eworker_t* ew, json_object* msg_attachs_json)
{
    if (msg_attachs_json == NULL)
    {
        warn("server_delete_attachments_json msg_attachs_json is NULL\n");
        return;
    }

    json_object* attach_json;
    json_object* hash_json;
    json_object* type_json;
    json_object* name_json;
    dbuser_file_t* file;
    const char* hash;
    const char* type;
    const char* name;
    size_t n;

    n = json_object_array_length(msg_attachs_json);

    for (size_t i = 0; i < n; i++)
    {
        file = calloc(1, sizeof(dbuser_file_t));

        attach_json = json_object_array_get_idx(msg_attachs_json, i);
        memset(file, 0, sizeof(dbuser_file_t));

        hash_json = json_object_object_get(attach_json, "hash");
        hash = json_object_get_string(hash_json);
        strncpy(file->hash, hash, DB_PFP_HASH_MAX);

        type_json = json_object_object_get(attach_json, "type");
        type = json_object_get_string(type_json);
        strncpy(file->mime_type, type, DB_MIME_TYPE_LEN);

        name_json = json_object_object_get(attach_json, "name");
        name = json_object_get_string(name_json);
        strncpy(file->name, name, DB_PFP_NAME_MAX);

        server_delete_file(ew, file);
    }

    json_object_put(msg_attachs_json);
}

static void 
server_delete_msg_attachments(eworker_t* ew, const char* attachments_str)
{
    json_object* msg_attachs_json;

    msg_attachs_json = json_tokener_parse(attachments_str);

    server_delete_attachments_json(ew, msg_attachs_json);
}

static void
server_msg_to_json(const dbmsg_t* dbmsg, json_object* json)
{
    json_object* attachments_json;

    attachments_json = json_tokener_parse(dbmsg->attachments);

    json_object_object_add(json, "cmd", 
                           json_object_new_string("group_msg"));
    json_object_object_add(json, "msg_id", 
                           json_object_new_int(dbmsg->msg_id));
    json_object_object_add(json, "group_id", 
                           json_object_new_int(dbmsg->group_id));
    json_object_object_add(json, "user_id", 
                           json_object_new_int(dbmsg->user_id));
    json_object_object_add(json, "content", 
                           json_object_new_string(dbmsg->content));
    json_object_object_add(json, "attachments", attachments_json);
    json_object_object_add(json, "timestamp", 
                           json_object_new_string(dbmsg->timestamp));
}

static void 
server_group_to_json(const dbgroup_t* dbgroup, json_object* json)
{
    json_object_object_add(json, "group_id", 
                           json_object_new_int(dbgroup->group_id));
    json_object_object_add(json, "owner_id",
                           json_object_new_int(dbgroup->owner_id));
    json_object_object_add(json, "name",
                           json_object_new_string(dbgroup->displayname));
    json_object_object_add(json, "desc",
                           json_object_new_string(dbgroup->desc));
    json_object_object_add(json, "public",
                           json_object_new_boolean(dbgroup->public));
}

static void 
server_send_group_codes(client_t* client, 
                        dbgroup_code_t* codes, 
                        u32 n_codes, 
                        u32 group_id, 
                        json_object* respond_json)
{
    json_object* codes_array_json;

    json_object_object_add(respond_json, "cmd", 
                           json_object_new_string("group_codes"));
    json_object_object_add(respond_json, "group_id",
                           json_object_new_int(group_id));
    json_object_object_add(respond_json, "codes",
                           json_object_new_array_ext(n_codes));
    codes_array_json = json_object_object_get(respond_json, "codes");

    for (u32 i = 0; i < n_codes; i++)
    {
        const dbgroup_code_t* invite_code = codes + i;

        json_object* code_json = json_object_new_object();
        json_object_object_add(code_json, "code",
                               json_object_new_string_len(invite_code->invite_code, 
                                                          DB_GROUP_CODE_MAX));
        json_object_object_add(code_json, "uses",
                               json_object_new_int(invite_code->uses));
        json_object_object_add(code_json, "max_uses",
                               json_object_new_int(invite_code->max_uses));

        json_object_array_add(codes_array_json, code_json);
    }

    ws_json_send(client, respond_json);
}

static const char* 
do_client_groups(UNUSED eworker_t* ew, dbcmd_ctx_t* ctx)
{
    dbgroup_t* groups = ctx->data;
    size_t n_groups = ctx->data_size;
    json_object* resp;

    if (ctx->ret == DB_ASYNC_ERROR)
        return "Failed to get groups";

    resp = json_object_new_object();

    json_object_object_add(resp, "cmd", 
            json_object_new_string("client_groups"));
    json_object_object_add(resp, "groups", 
                json_object_new_array_ext(n_groups));
    json_object* group_array_json = json_object_object_get(resp, 
                "groups");

    for (size_t i = 0; i < n_groups; i++)
    {
        const dbgroup_t* g = groups + i;
        json_object* group_json = json_object_new_object();

        server_group_to_json(g, group_json);
        json_object_array_add(group_array_json, group_json);
    }

    ws_json_send(ctx->client, resp);

    json_object_put(resp);

    return NULL;
}

const char* 
server_group_create(eworker_t* ew, 
                    client_t* client, 
                    json_object* payload, 
                    UNUSED json_object* respond_json)
{
    json_object* name_json;
    json_object* public_json;
    const char* name;
    bool public_group;
    u32 owner_id;
    dbgroup_t* group;
    
    RET_IF_JSON_BAD(name_json, payload, "name", json_type_string);
    RET_IF_JSON_BAD(public_json, payload, "public", json_type_boolean);

    name = json_object_get_string(name_json);
    public_group = json_object_get_boolean(public_json);
    owner_id = client->dbuser->user_id;
    group = calloc(1, sizeof(dbgroup_t));

    group->owner_id = owner_id;
    group->public = public_group;
    strncpy(group->displayname, name, DB_DISPLAYNAME_MAX);

    dbcmd_ctx_t ctx = {
        .exec = do_client_groups,
        .data_size = 1
    };
    if (!db_async_create_group(&ew->db, group, &ctx))
        return "Internal error: async-create-group";
    return NULL;
}

const char* 
server_client_groups(eworker_t* ew, client_t* client, 
                     UNUSED json_object* payload, 
                     UNUSED json_object* respond_json) 
{
    dbcmd_ctx_t ctx = {
        .exec = do_client_groups,
    };
    if (db_async_get_user_groups(&ew->db, client->dbuser->user_id, &ctx) == false)
        return "Internal error: async-get-user-groups";
    return NULL;
}

static const char* 
get_all_groups_result(UNUSED eworker_t* ew, dbcmd_ctx_t* ctx)
{
    const char* group_array_json;
    json_object* resp;

    if (ctx->ret == DB_ASYNC_ERROR)
        return "Failed to get public groups";

    group_array_json = ctx->param.str;
    resp = json_object_new_object();

    json_object_object_add(resp, "cmd",
                           json_object_new_string("get_all_groups"));
    json_object_object_add(resp, "groups",
                           json_tokener_parse(group_array_json));

    ws_json_send(ctx->client, resp);
    json_object_put(resp);

    return NULL;
}

const char* 
server_get_all_groups(eworker_t* ew, 
                      UNUSED client_t* client, 
                      UNUSED json_object* payload, 
                      UNUSED json_object* respond_json)
{
    // TODO: Limit getting public groups.
    dbcmd_ctx_t ctx = {
        .exec = get_all_groups_result
    };
    if (!db_async_get_public_groups(&ew->db, client->dbuser->user_id, &ctx))
        return "Internal error: async-get-public-groups";

    return NULL;
}

static void
on_user_group_join(eworker_t* ew, client_t* client, u32 group_id)
{
    json_object* online_clients_respond;

    dbcmd_ctx_t ctx = {
        .exec = do_client_groups,
        .client = client
    };
    db_async_get_group(&ew->db, group_id, &ctx);
    
    online_clients_respond = json_object_new_object();
    json_object_object_add(online_clients_respond, "cmd",
                           json_object_new_string("join_group"));
    json_object_object_add(online_clients_respond, "user_id", 
                           json_object_new_int(client->dbuser->user_id));
    json_object_object_add(online_clients_respond, "group_id", 
                           json_object_new_int(group_id));    

    server_group_broadcast(ew, group_id, online_clients_respond);
}

static const char*
join_group_result(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    u32 group_id;

    if (ctx->ret == DB_ASYNC_ERROR)
        return "Failed to join group";

    group_id = ctx->param.group_id;
    
    on_user_group_join(ew, ctx->client, group_id);

    return NULL;
}

const char* 
server_join_group(eworker_t* ew, 
                  client_t* client, 
                  json_object* payload, 
                  UNUSED json_object* respond_json)
{
    json_object* group_id_json;
    u32 group_id;

    RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);
    group_id = json_object_get_int(group_id_json);

    dbcmd_ctx_t ctx = {
        .exec = join_group_result,
        .param.group_id = group_id
    };

    if (!db_async_user_join_pub_group(&ew->db, client->dbuser->user_id, group_id, &ctx))
        return "Internal error: async-user-join-pub-group";

    return NULL;
}

const char*
server_get_send_group_msg(eworker_t* ew, 
                          const dbmsg_t* dbmsg)
{
    json_object* respond_json;

    respond_json = json_object_new_object();

    server_msg_to_json(dbmsg, respond_json);

    // Update all online group members
    server_group_broadcast(ew, dbmsg->group_id, respond_json);

    return NULL;
}

static const char*
server_verify_msg_attachments(json_object* attachments_json, size_t n)
{
    json_object* attach;
    json_object* name_json;
    json_object* type_json;
    const char*  type;

    for (size_t i = 0; i < n; i++)
    {
        attach = json_object_array_get_idx(attachments_json, i);

        RET_IF_JSON_BAD(name_json, attach, "name", json_type_string);
        RET_IF_JSON_BAD(type_json, attach, "type", json_type_string);

        type = json_object_get_string(type_json);
        if (!str_startwith(type, "image/"))
            return "Attachment type is not image.";
    }

    return NULL;
}

static void
set_msg(dbmsg_t* msg, u32 user_id, u32 group_id, const char* content)
{
    if (!msg)
        return;

    msg->user_id = user_id;
    msg->group_id = group_id;
    strncpy(msg->content, content, DB_MESSAGE_MAX);
}

static const char*
do_group_msg(UNUSED eworker_t* ew, dbcmd_ctx_t* ctx)
{
    dbmsg_t* msg;

    if (ctx->ret == DB_ASYNC_ERROR)
        return "Failed to insert message";

    msg = ctx->data;

    server_get_send_group_msg(ew, msg);

    return NULL;
}

const char* 
server_group_msg(eworker_t* ew, client_t* client, 
                 json_object* payload, UNUSED json_object* respond_json)
{
    json_object* group_id_json;
    json_object* content_json;
    json_object* attachments_json;
    u32 group_id;
    const char* content;
    u32 user_id;
    size_t n_attachments;
    const char* errmsg = NULL;
    dbmsg_t* msg;
    upload_token_t* ut;

    RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);
    RET_IF_JSON_BAD(content_json, payload, "content", json_type_string);
    RET_IF_JSON_BAD(attachments_json, payload, "attachments", json_type_array);

    group_id = json_object_get_int(group_id_json);
    content = json_object_get_string(content_json);
    user_id = client->dbuser->user_id;
    n_attachments = json_object_array_length(attachments_json);

    if (n_attachments == 0)
    {
        /*
         *  If Message has no attachments insert message into database,
         *  and send message to all clients in group.
         */
        msg = calloc(1, sizeof(dbmsg_t));
        set_msg(msg, user_id, group_id, content);

        dbcmd_ctx_t ctx = {
            .exec = do_group_msg
        };

        if (!db_async_insert_group_msg(&ew->db, msg, &ctx))
        {
            free(msg);
            errmsg = "Internal error: async-group-msg-insert";
        }
    }
    else
    {
        if ((errmsg = server_verify_msg_attachments(attachments_json, n_attachments)))
            return errmsg;

        /*
         * If message has attachments, create upload token (session), 
         * save ewe message temporarily, send the upload token to client,
         * and wait for client to send attachments via HTTP POST,
         * ewen insert the message into database.
         */
        ut = server_new_upload_token_attach(ew);
        msg = &ut->msg_state.msg;
        set_msg(msg, user_id, group_id, content);
        msg->attachments_json = json_object_get(attachments_json);
        ut->msg_state.total = n_attachments;

        server_send_upload_token(client, "send_attachments", ut);
    }
    return errmsg;
}

static const char*
do_get_group_msgs(UNUSED eworker_t* ew, dbcmd_ctx_t* ctx)
{
    json_object* resp;
    u32 group_id;
    const char* msgs_array_json;

    if (ctx->ret == DB_ASYNC_ERROR)
        return "Failed to get group messages";

    resp = json_object_new_object();
    group_id = ctx->param.group_msgs.group_id;
    msgs_array_json = ctx->param.group_msgs.msgs_json;

    json_object_object_add(resp, "cmd",
                           json_object_new_string("get_group_msgs"));
    json_object_object_add(resp, "group_id",
                           json_object_new_int(group_id));
    json_object_object_add(resp, "messages",
                           json_tokener_parse(msgs_array_json));

    ws_json_send(ctx->client, resp);

    json_object_put(resp);

    return NULL;
}

const char* 
server_get_group_msgs(UNUSED eworker_t* ew, 
                      UNUSED client_t* client, 
                      json_object* payload, 
                      UNUSED json_object* respond_json)
{
    json_object* limit_json;
    json_object* group_id_json;
    json_object* offset_json;
    u64 group_id;
    u32 limit;
    u32 offset;

    RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);
    RET_IF_JSON_BAD(limit_json, payload, "limit", json_type_int);
    RET_IF_JSON_BAD(offset_json, payload, "offset", json_type_int);

    group_id = json_object_get_int(group_id_json);
    limit = json_object_get_int(limit_json);
    offset = json_object_get_int(offset_json);

    dbcmd_ctx_t ctx = {
        .exec = do_get_group_msgs,
        .param.group_id = group_id
    };

    if (!db_async_get_group_msgs(&ew->db, group_id, limit, offset, &ctx))
        return "Internal error: async-get-group-msgs";

    return NULL;
}

static const char*
create_group_code_result(UNUSED eworker_t* ew, dbcmd_ctx_t* ctx)
{
    dbgroup_code_t* group_code;
    json_object* resp;

    if (ctx->ret == DB_ASYNC_ERROR)
        return "Failed to create group code";

    group_code = ctx->data;
    resp = json_object_new_object();
    server_send_group_codes(ctx->client, group_code, 1, group_code->group_id, resp);
    json_object_put(resp);

    return NULL;
}

const char* 
server_create_group_code(eworker_t* ew, 
                         client_t* client,
                         json_object* payload, 
                         UNUSED json_object* respond_json)
{
    json_object* group_id_json;
    json_object* max_uses_json;
    dbgroup_code_t* group_code;
    u32 group_id;
    i32 max_uses;

    RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);
    RET_IF_JSON_BAD(max_uses_json, payload, "max_uses", json_type_int);

    group_id = json_object_get_int(group_id_json);
    max_uses = json_object_get_int(max_uses_json);
    if (max_uses == 0)
        max_uses = 1;
    group_code = calloc(1, sizeof(dbgroup_code_t));
    group_code->group_id = group_id;
    group_code->max_uses = max_uses;

    dbcmd_ctx_t ctx = {
        .exec = create_group_code_result,
    };

    if (!db_async_create_group_code(&ew->db, group_code, client->dbuser->user_id, &ctx))
        return "Error: async-create-group-code";
    return NULL;
}

static const char*
join_group_code_result(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    u32 group_id;

    if (ctx->ret == DB_ASYNC_ERROR)
        return "Failed to join group";

    group_id = ctx->param.group_id;
    on_user_group_join(ew, ctx->client, group_id);
    return NULL;
}

const char* 
server_join_group_code(UNUSED eworker_t* ew, 
                       UNUSED client_t* client,
                       UNUSED json_object* payload, 
                       UNUSED json_object* respond_json)
{
    json_object* code_json;
    const char* code;
    const u32 user_id = client->dbuser->user_id;
    
    RET_IF_JSON_BAD(code_json, payload, "code", json_type_string);
    code = json_object_get_string(code_json);

    dbcmd_ctx_t ctx = {
        .exec = join_group_code_result
    };

    if (!db_async_user_join_group_code(&ew->db, code, user_id, &ctx))
        return "Error: async-user-join-group-code";
    return NULL;
}

static const char*
get_group_codes_result(UNUSED eworker_t* ew, dbcmd_ctx_t* ctx)
{
    u32 group_id;
    const char* group_codes_array_json;
    json_object* resp;

    if (ctx->ret == DB_ASYNC_ERROR)
        return "Failed to get group codes";

    group_id = ctx->param.group_codes.group_id;
    group_codes_array_json = ctx->param.group_codes.array_json;
    resp = json_object_new_object();

    json_object_object_add(resp, "cmd", 
                           json_object_new_string("group_codes"));
    json_object_object_add(resp, "group_id",
                           json_object_new_int(group_id));
    json_object_object_add(resp, "codes",
                           json_tokener_parse(group_codes_array_json));

    ws_json_send(ctx->client, resp);
     
    json_object_put(resp);
    return NULL;
}

const char* 
server_get_group_codes(eworker_t* ew, 
                       client_t* client,
                       json_object* payload, 
                       UNUSED json_object* respond_json)
{
    json_object* group_id_json;
    u32 group_id;

    RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);
    group_id = json_object_get_int(group_id_json);

    dbcmd_ctx_t ctx = {
        .exec = get_group_codes_result,
    };

    if (!db_async_get_group_codes(&ew->db, group_id, client->dbuser->user_id, &ctx))
        return "Error: async-get-group-codes";

    return NULL;
}

static const char*
delete_group_code_result(UNUSED eworker_t* ew, dbcmd_ctx_t* ctx)
{
    if (ctx->ret == DB_ASYNC_ERROR)
        return "Failed to delete group code";
    return NULL;
}

const char* 
server_delete_group_code(eworker_t* ew, client_t* client, 
                         json_object* payload, UNUSED json_object* resp)
{
    json_object* code_json;
    u32 user_id = client->dbuser->user_id;
    const char* code;

    RET_IF_JSON_BAD(code_json, payload, "code", json_type_string);
    code = json_object_get_string(code_json);

    dbcmd_ctx_t ctx = {
        .exec = delete_group_code_result
    };
    if (!db_async_delete_group_code(&ew->db, code, user_id, &ctx))
        return "Error: async-delete-group-code";
    return NULL;
}

static const char* 
delete_msg_result(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    u32 msg_id;
    u32 group_id;
    json_object* resp;
    const char* attachments;

    if (ctx->ret == DB_ASYNC_ERROR)
        return "Failed to delete message";

    msg_id = ctx->param.del_msg.msg_id;
    group_id = ctx->param.del_msg.group_id;
    attachments = ctx->param.del_msg.attachments_json;

    server_delete_msg_attachments(ew, attachments);

    resp = json_object_new_object();
    json_object_object_add(resp, "cmd",
                           json_object_new_string("delete_msg"));
    json_object_object_add(resp, "msg_id",
                           json_object_new_int(msg_id));
    json_object_object_add(resp, "group_id",
                           json_object_new_int(group_id));
    server_group_broadcast(ew, group_id, resp);

    return NULL;
}

const char* 
server_delete_group_msg(UNUSED eworker_t* ew, 
                        UNUSED client_t* client,
                        UNUSED json_object* payload, 
                        UNUSED json_object* respond_json)
{
    json_object* msg_id_json;
    u32 msg_id;
    u32 user_id = client->dbuser->user_id;

    RET_IF_JSON_BAD(msg_id_json, payload, "msg_id", json_type_int);
    msg_id = json_object_get_int(msg_id_json);

    dbcmd_ctx_t ctx = {
        .exec = delete_msg_result,
        .param.del_msg.msg_id = msg_id
    };
    if (!db_async_delete_msg(&ew->db, msg_id, user_id, &ctx))
        return "Error: async-delete-msg";
    return NULL;
}

static const char* 
delete_group_result(UNUSED eworker_t* ew, dbcmd_ctx_t* ctx)
{
    json_object* resp;
    dbcmd_ctx_t* base = ctx;
    dbcmd_ctx_t* attach_ctx = ctx->next;
    dbcmd_ctx_t* member_ids_ctx = attach_ctx->next;
    json_object** attach_array;
    size_t        attach_array_len;
    u32*        member_ids;
    size_t      n_members;
    u32 group_id;
    group_id = ctx->param.group_id;

    if (group_id == 0)
    {
        warn("group_id is 0!\n");
    }

    while (ctx)
    {
        debug("group_id: %u -- ctx->ret: %d\n", group_id, ctx->ret);
        if (ctx->ret == DB_ASYNC_ERROR)
            return "Failed to delete group";
        ctx = ctx->next;
    }
    ctx = base;

    attach_array = attach_ctx->data;
    attach_array_len = attach_ctx->data_size;

    member_ids = member_ids_ctx->data;
    n_members = member_ids_ctx->data_size;

    for (size_t i = 0; i < attach_array_len; i++)
    {
        json_object* attach_json = attach_array[i];
        server_delete_attachments_json(ew, attach_json);
    }

    resp = json_object_new_object();
    json_object_object_add(resp, "cmd", 
                           json_object_new_string("delete_group"));
    json_object_object_add(resp, "group_id",
                           json_object_new_int(group_id));

    for (size_t i = 0; i < n_members; i++)
    {
        client_t* member_client = server_get_client_user_id(ew->server, member_ids[i]);
        if (member_client)
            ws_json_send(member_client, resp);
    }

    json_object_put(resp);
    return NULL;
}

static const char* 
get_group_owner(eworker_t* ew, dbcmd_ctx_t* gowner_ctx)
{
    u32 owner_id;
    u32 group_id;
    u32 user_id;

    if (gowner_ctx->ret == DB_ASYNC_ERROR)
        return "Failed to get group owner";

    owner_id = gowner_ctx->param.group_owner.owner_id;
    group_id = gowner_ctx->param.group_owner.group_id;
    user_id =  gowner_ctx->param.group_owner.user_id;

    if (user_id != owner_id)
        return "Not group owner";

    dbcmd_ctx_t ctx = {
        .exec = delete_group_result,
        .param.group_id = group_id
    };

    if (!db_async_delete_group(&ew->db, group_id, &ctx))
        return "Error: async-delete-group";
    return NULL;
}

const char* 
server_delete_group(eworker_t* ew, 
                    UNUSED client_t* client, 
                    json_object* payload, 
                    UNUSED json_object* resp_json)
{
    json_object* group_id_json;
    u32 group_id;

    RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);
    group_id = json_object_get_int(group_id_json);

    dbcmd_ctx_t ctx = {
        .exec = get_group_owner,
        .param.group_owner.group_id = group_id,
        .param.group_owner.user_id = client->dbuser->user_id,
    };
    if (!db_async_get_group_owner(&ew->db, group_id, &ctx))
        return "Error: async-get-group-owner";
    return NULL;
}

static const char*
do_get_group_member_ids(UNUSED eworker_t* ew, dbcmd_ctx_t* ctx)
{
    u32 group_id;
    const char* member_ids_str;
    json_object* resp;

    if (ctx->ret == DB_ASYNC_ERROR)
        return "Failed to get group member IDs";

    group_id = ctx->param.member_ids.group_id;
    member_ids_str = ctx->param.member_ids.ids_json;
    resp = json_object_new_object();
    json_object_object_add(resp, "cmd",
                           json_object_new_string("get_member_ids"));
    json_object_object_add(resp, "group_id",
                           json_object_new_int(group_id));
    json_object_object_add(resp, "member_ids",
                           json_tokener_parse(member_ids_str));

    ws_json_send(ctx->client, resp);

    json_object_put(resp);

    return NULL;
}

const char* 
server_get_group_member_ids(eworker_t* ew,
                            UNUSED client_t* client, 
                            json_object* payload, 
                            UNUSED json_object* resp_json)
{
    json_object* group_id_json;
    u32 group_id;

    RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);

    group_id = json_object_get_int(group_id_json);
    dbcmd_ctx_t ctx = {
        .exec = do_get_group_member_ids,
        .param.member_ids.group_id = group_id
    };

    if (db_async_get_group_member_ids(&ew->db, group_id, &ctx) == false) 
        return "internal error: async-get-group-member-ids";
    return NULL;
}
