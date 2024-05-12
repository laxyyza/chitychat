#include "chat/group.h"
#include "chat/db.h"
#include "chat/db_group.h"
#include "chat/ws_text_frame.h"
#include "server_websocket.h"

static void 
server_group_broadcast(eworker_t* ew, u32 group_id, json_object* json)
{
    const dbuser_t* member;
    client_t* member_client;
    dbuser_t* gmembers;
    u32 n_members;

    gmembers = server_db_get_group_members(&ew->db, group_id, &n_members);

    for (u32 i = 0; i < n_members; i++)
    {
        member = gmembers + i;
        member_client = server_get_client_user_id(ew->server, member->user_id);

        if (member_client)
            ws_json_send(member_client, json);
    }

    free(gmembers);
}

UNUSED static void 
server_delete_msg_attachments(eworker_t* ew, dbmsg_t* msg)
{
    json_object* attach_json;
    json_object* hash_json;
    json_object* type_json;
    json_object* name_json;
    dbuser_file_t file;
    const char* hash;
    const char* type;
    const char* name;
    size_t n;
    bool put_json = false;

    if (msg->attachments_json == NULL)
    {
        msg->attachments_json = json_tokener_parse(msg->attachments);
        put_json = true;
    }

    n = json_object_array_length(msg->attachments_json);

    for (size_t i = 0; i < n; i++)
    {
        attach_json = json_object_array_get_idx(msg->attachments_json, i);
        memset(&file, 0, sizeof(dbuser_file_t));

        hash_json = json_object_object_get(attach_json, "hash");
        hash = json_object_get_string(hash_json);
        strncpy(file.hash, hash, DB_PFP_HASH_MAX);

        type_json = json_object_object_get(attach_json, "type");
        type = json_object_get_string(type_json);
        strncpy(file.mime_type, type, DB_MIME_TYPE_LEN);

        name_json = json_object_object_get(attach_json, "name");
        name = json_object_get_string(name_json);
        strncpy(file.name, name, DB_PFP_NAME_MAX);

        server_delete_file(ew, &file);
    }

    if (put_json)
        json_object_put(msg->attachments_json);
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
server_group_to_json(eworker_t* ew, const dbgroup_t* dbgroup, json_object* json)
{
    u32 n_members;
    dbuser_t* gmembers;
    json_object* members_array_json;

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

    if (ew)
    {
        gmembers = server_db_get_group_members(&ew->db, dbgroup->group_id, &n_members);
        
        json_object_object_add(json, "members_id", 
                               json_object_new_array_ext(n_members));
        members_array_json = json_object_object_get(json, "members_id");

        // TODO: Add full user info in packet
        for (u32 m = 0; m < n_members; m++)
        {
            const dbuser_t* member = gmembers + m;
            json_object_array_add(members_array_json, 
                    json_object_new_int(member->user_id));
        }

        if (gmembers)
            free(gmembers);
    }
}

static void 
server_send_group_codes(client_t* client, dbgroup_code_t* codes, 
                        u32 n_codes, u32 group_id, json_object* respond_json)
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

const char* 
server_group_create(eworker_t* ew, client_t* client, json_object* payload, 
                    json_object* respond_json)
{
    json_object* name_json;
    json_object* public_json;
    json_object* group_array_json;
    json_object* group_json;
    dbgroup_t new_group;
    const char* name;
    bool public_group;

    RET_IF_JSON_BAD(name_json, payload, "name", json_type_string);
    RET_IF_JSON_BAD(public_json, payload, "public", json_type_boolean);

    name = json_object_get_string(name_json);
    public_group = json_object_get_boolean(public_json);

    memset(&new_group, 0, sizeof(dbgroup_t));
    new_group.owner_id = client->dbuser->user_id;
    new_group.public = public_group;
    strncpy(new_group.displayname, name, DB_DISPLAYNAME_MAX);

    if (!server_db_insert_group(&ew->db, &new_group))
        return "Failed to create group";

    info("Creating new group, id: %u, name: '%s', owner_id: %u\n", 
                new_group.group_id, name, new_group.owner_id);

    json_object_object_add(respond_json, "cmd", 
            json_object_new_string("client_groups"));
    json_object_object_add(respond_json, "groups", 
            json_object_new_array_ext(1));
    group_array_json = json_object_object_get(respond_json, "groups");

    group_json = json_object_new_object();
    server_group_to_json(ew, &new_group, group_json);
    json_object_array_add(group_array_json, group_json);

    ws_json_send(client, respond_json);

    return NULL;
}

static const char* 
do_client_groups(eworker_t* ew, dbcmd_ctx_t* ctx)
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

        server_group_to_json(ew, g, group_json);
        json_object_array_add(group_array_json, group_json);
    }

    ws_json_send(ctx->client, resp);

    json_object_put(resp);

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

const char* 
server_get_all_groups(eworker_t* ew, client_t* client, 
                      UNUSED json_object* payload, json_object* respond_json)
{
    // TODO: Limit getting public groups.
    u32 n_groups;
    dbgroup_t* groups = server_db_get_public_groups(&ew->db, client->dbuser->user_id, &n_groups);
    if (!groups)
        return "Failed to get groups";

    json_object_object_add(respond_json, "cmd", 
                           json_object_new_string("get_all_groups"));
    json_object_object_add(respond_json, "groups",
                           json_object_new_array_ext(n_groups));
    json_object* groups_json = json_object_object_get(respond_json, "groups");

    for (size_t i = 0; i < n_groups; i++)
    {
        dbgroup_t* g = groups + i;
        json_object* group_json = json_object_new_object();

        server_group_to_json(NULL, g, group_json);
        json_object_array_add(groups_json, group_json);
    }

    ws_json_send(client, respond_json);

    free(groups);

    return NULL;
}

static void
on_user_group_join(eworker_t* ew, client_t* client,
                   dbgroup_t* group, json_object* respond_json)
{
    json_object* group_json;
    json_object* array_json;
    json_object* members_array_json;
    json_object* oewer_clients_respond;
    dbuser_t* gmembers;
    const dbuser_t* member;
    client_t* member_client;
    u32 n_members;

    /*
     * To client who joined. (groups array always length of 1)
     * {
     *      "cmd": "client_groups",
     *      "groups": [
     *          {
     *              "group_id": <type:int>,
     *              "owner_id": <type:int>,
     *              "name":     <type:string>,
     *              "desc":     <type:string>,
     *              "public":   <type:bool>
     *              "members_id": [ <type:int[]> ]
     *          }
     *      ]
     * }
     */

    json_object_object_add(respond_json, "cmd", 
                           json_object_new_string("client_groups"));
    json_object_object_add(respond_json, "groups", 
                           json_object_new_array_ext(1));
    array_json = json_object_object_get(respond_json, "groups");
    group_json = json_object_new_object();

    server_group_to_json(NULL, group, group_json);

    gmembers = server_db_get_group_members(&ew->db, group->group_id, &n_members);

    json_object_object_add(group_json, "members_id", 
                           json_object_new_array_ext(n_members));
    members_array_json = json_object_object_get(group_json, "members_id");

    /*
     * To oewer clients who are in the group.
     * {
     *      "cmd":      "join_group",
     *      "user_id":  <type:int>,
     *      "group_id": <type:int>
     * }
     */

    oewer_clients_respond = json_object_new_object();
    json_object_object_add(oewer_clients_respond, "cmd",
                           json_object_new_string("join_group"));
    json_object_object_add(oewer_clients_respond, "user_id", 
                           json_object_new_int(client->dbuser->user_id));
    json_object_object_add(oewer_clients_respond, "group_id", 
                           json_object_new_int(group->group_id));

    for (u32 m = 0; m < n_members; m++)
    {
        member = gmembers + m;
        member_client = server_get_client_user_id(ew->server, member->user_id);

        if (member_client && member_client != client)
            ws_json_send(member_client, oewer_clients_respond);

        json_object_array_add(members_array_json, 
                json_object_new_int(member->user_id));
    }

    json_object_put(oewer_clients_respond);
    json_object_array_add(array_json, group_json);

    ws_json_send(client, respond_json);

    free(gmembers);
}

const char* 
server_join_group(UNUSED eworker_t* ew, 
                  UNUSED client_t* client, 
                  UNUSED json_object* payload, 
                  UNUSED json_object* respond_json)
{
    return "server_join_group not implemented!";
    // json_object* group_id_json;
    // u32 group_id;
    // dbgroup_t* group;
    //
    // RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);
    // group_id = json_object_get_int(group_id_json);
    //
    // group = server_db_get_group(&ew->db, group_id);
    // if (!group)
    //     return "Group not found";
    //
    // if (!server_db_insert_group_member(&ew->db, group->group_id, 
    //                                    client->dbuser->user_id))
    // {
    //     free(group);
    //     return "Failed to join";
    // }
    //
    // on_user_group_join(ew, client, group, respond_json);
    //
    // free(group);
    //
    // return NULL;
}

const char*
server_get_send_group_msg(eworker_t* ew, 
                          const dbmsg_t* dbmsg, 
                          const u32 group_id)
{
    json_object* respond_json;

    respond_json = json_object_new_object();

    server_msg_to_json(dbmsg, respond_json);

    // Update all oewer online group members
    server_group_broadcast(ew, group_id, respond_json);

    json_object_put(respond_json);

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
    dbmsg_t new_msg;
    const char* errmsg;

    RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);
    RET_IF_JSON_BAD(content_json, payload, "content", json_type_string);
    RET_IF_JSON_BAD(attachments_json, payload, "attachments", json_type_array);

    group_id = json_object_get_int(group_id_json);
    content = json_object_get_string(content_json);
    user_id = client->dbuser->user_id;
    n_attachments = json_object_array_length(attachments_json);

    memset(&new_msg, 0, sizeof(dbmsg_t));
    new_msg.user_id = user_id;
    new_msg.group_id = group_id;
    strncpy(new_msg.content, content, DB_MESSAGE_MAX);

    if (n_attachments == 0)
    {
        /*
         *  If Message has no attachments insert message into database,
         *  and send message to all clients in group.
         */

        if (!server_db_insert_msg(&ew->db, &new_msg))
            return "Failed to send message";

        errmsg = server_get_send_group_msg(ew, &new_msg, group_id);
        if (new_msg.attachments_inheap)
            free(new_msg.attachments);
        return errmsg;
    }
    else
    {
        if ((errmsg = server_verify_msg_attachments(attachments_json, n_attachments)))
            return errmsg;

        /*
         * If message has attachments, create upload token (session), 
         * save ewe message temporarily, send the upload token to client,
         * and wait for client to send ewe attachments via HTTP POST,
         * ewen insert the message into database.
         */

        upload_token_t* ut = server_new_upload_token_attach(ew, &new_msg);
        ut->msg_state.msg.attachments_json = json_object_get(attachments_json);
        ut->msg_state.total = n_attachments;

        server_send_upload_token(client, "send_attachments", ut);

        return NULL;
    }
}

const char* 
server_get_group_msgs(eworker_t* ew, client_t* client, 
               json_object* payload, json_object* respond_json)
{
    json_object* limit_json;
    json_object* group_id_json;
    json_object* offset_json;
    json_object* msgs_json;
    u64 group_id;
    u32 limit;
    u32 offset;
    u32 n_msgs;
    dbmsg_t* group_msgs;
    dbmsg_t* msg;
    json_object* msg_in_array;

    RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);
    RET_IF_JSON_BAD(limit_json, payload, "limit", json_type_int);
    RET_IF_JSON_BAD(offset_json, payload, "offset", json_type_int);

    group_id = json_object_get_int(group_id_json);
    limit = json_object_get_int(limit_json);
    offset = json_object_get_int(offset_json);

    group_msgs = server_db_get_msgs_from_group(&ew->db, group_id, limit, 
                                               offset, &n_msgs);

    json_object_object_add(respond_json, "cmd", 
                           json_object_new_string("get_group_msgs"));
    json_object_object_add(respond_json, "group_id", 
                           json_object_new_int(group_id));
    json_object_object_add(respond_json, "messages", 
                           json_object_new_array_ext(n_msgs));
    msgs_json = json_object_object_get(respond_json, "messages");

    for (u32 i = 0; i < n_msgs; i++)
    {
        msg = group_msgs + i;

        msg_in_array = json_object_new_object();

        server_msg_to_json(msg, msg_in_array);

        json_object_array_add(msgs_json, msg_in_array);

        if (msg->attachments_inheap && msg->attachments)
        {
            free(msg->attachments);
            msg->attachments_inheap = false;
        }
    }

    ws_json_send(client, respond_json);

    free(group_msgs);
    return NULL;
}

const char* 
server_create_group_code(eworker_t* ew, client_t* client,
                         json_object* payload, json_object* respond_json)
{
    json_object* group_id_json;
    json_object* max_uses_json;
    dbgroup_code_t group_code;
    u32 group_id;
    i32 max_uses;

    RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);
    RET_IF_JSON_BAD(max_uses_json, payload, "max_uses", json_type_int);

    group_id = json_object_get_int(group_id_json);
    max_uses = json_object_get_int(max_uses_json);
    if (max_uses == 0)
        max_uses = 1;

    if (!client->dbuser || !server_db_user_in_group(&ew->db, group_id, 
                                                    client->dbuser->user_id))
        return "Not a group member";

    group_code.group_id = group_id;
    group_code.max_uses = max_uses;
    group_code.uses = 0;

    if (!server_db_insert_group_code(&ew->db, &group_code))
        return "Failed to create group code";

    server_send_group_codes(client, &group_code, 1, group_id, respond_json);

    return NULL;
}

const char* 
server_join_group_code(eworker_t* ew, client_t* client,
                       json_object* payload, json_object* respond_json)
{
    json_object* code_json;
    const char* code;
    const char* errmsg = NULL;
    const u32 user_id = client->dbuser->user_id;
    dbgroup_t* group_joined;

    RET_IF_JSON_BAD(code_json, payload, "code", json_type_string);
    code = json_object_get_string(code_json);

    group_joined = server_db_insert_group_member_code(&ew->db,
                                                      code,
                                                      user_id);
    if (!group_joined)
        return "Failed to join or already joined";

    on_user_group_join(ew, client, group_joined, respond_json);
        
    free(group_joined);
    return errmsg;
}

const char* 
server_get_group_codes(eworker_t* ew, client_t* client,
                       json_object* payload, json_object* respond_json)
{
    json_object* group_id_json;
    u32 group_id;
    dbgroup_code_t* codes;
    u32 n_codes;

    RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);
    group_id = json_object_get_int(group_id_json);

    if (!server_db_user_in_group(&ew->db, group_id, client->dbuser->user_id))
        return "Not a group member";

    codes = server_db_get_all_group_codes(&ew->db, group_id, &n_codes);

    server_send_group_codes(client, codes, n_codes, group_id, respond_json);

    if (codes)
        free(codes);
    return NULL;
}

const char* 
server_delete_group_code(eworker_t* ew, client_t* client, 
                         json_object* payload, UNUSED json_object* resp)
{
    json_object* code_json;
    json_object* group_id_json;
    u32 group_id;
    u32 user_id = client->dbuser->user_id;
    const char* code;

    RET_IF_JSON_BAD(code_json, payload, "code", json_type_string);
    RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);

    code = json_object_get_string(code_json);
    group_id = json_object_get_int(group_id_json);

    if (!server_db_user_in_group(&ew->db, group_id, user_id)) 
        return "Not a group owner";

    if (!server_db_delete_group_code(&ew->db, code))
        return "Failed to delete group code";

    return NULL;
}

const char* 
server_delete_group_msg(UNUSED eworker_t* ew, 
                        UNUSED client_t* client,
                        UNUSED json_object* payload, 
                        UNUSED json_object* respond_json)
{
    return "delete_group_msg not implemented!";
//     json_object* msg_id_json;
//     u32 msg_id;
//     dbmsg_t* msg_to_delete;
//     dbgroup_t* group;
//     const char* errmsg = NULL;
//
//     RET_IF_JSON_BAD(msg_id_json, payload, "msg_id", json_type_int);
//     msg_id = json_object_get_int(msg_id_json);
//
//     msg_to_delete = server_db_get_msg(&ew->db, msg_id);
//     if (!msg_to_delete)
//         return "Message not found";
//
//     group = server_db_get_group(&ew->db, msg_to_delete->group_id);
//     if (!group)
//     {
//         errmsg = "Group not found";
//         goto cleanup;
//     }
//
//     /*
//      * Currently only group owner and user's own can delete messages.
//      * Will change, when member roles (admin) are implemented.
//      */
//     if (client->dbuser->user_id != group->owner_id &&
//         client->dbuser->user_id != msg_to_delete->user_id)
//     {
//         errmsg = "Permission denied";
//         goto cleanup;
//     } 
//
//     if (!server_db_delete_msg(&ew->db, msg_to_delete->msg_id))
//     {
//         errmsg = "Failed to delete message";
//         goto cleanup;
//     }
//
//     json_object_object_add(respond_json, "cmd", 
//                            json_object_new_string("delete_msg"));
//     json_object_object_add(respond_json, "group_id",
//                            json_object_new_int(msg_to_delete->group_id));
//     json_object_object_add(respond_json, "msg_id",
//                            json_object_new_int(msg_to_delete->msg_id));
//
//     server_group_broadcast(ew, group->group_id, respond_json);
//     server_delete_msg_attachments(ew, msg_to_delete);
//
// cleanup:
//     free(msg_to_delete);
//     free(group);
//
//     return errmsg;
}

const char* 
server_delete_group(UNUSED eworker_t* ew, 
                    UNUSED client_t* client, 
                    UNUSED json_object* payload, 
                    UNUSED json_object* resp_json)
{
    return "server_delete_group not implemented!";
    // json_object* group_id_json;
    // const dbuser_t* member;
    // client_t* member_client;
    // dbuser_t* gmembers;
    // dbmsg_t* attach_msgs;
    // u32 n_members;
    // u32 n_msgs;
    // dbgroup_t* group;
    // u32 group_id;
    // u32 owner_id;
    //
    // RET_IF_JSON_BAD(group_id_json, payload, "group_id", json_type_int);
    // group_id = json_object_get_int(group_id_json);
    //
    // if ((group = server_db_get_group(&ew->db, group_id)) == NULL)
    //     return "Group not found";
    //
    // owner_id = group->owner_id;
    // free(group);
    //
    // if (owner_id != client->dbuser->user_id)
    //     return "Permission denied";
    //
    // gmembers = server_db_get_group_members(&ew->db, group_id, &n_members);
    // attach_msgs = server_db_get_msgs_only_attachs(&ew->db, group_id, &n_msgs);
    // 
    // if (!server_db_delete_group(&ew->db, group_id))
    // {
    //     free(gmembers);
    //     return "Failed to delete group";
    // }
    //
    // for (u32 i = 0; i < n_msgs; i++)
    // {
    //     dbmsg_t* msg = attach_msgs + i;
    //     server_delete_msg_attachments(ew, msg);
    //     json_object_put(msg->attachments_json);
    // }
    // free(attach_msgs);
    //
    // json_object_object_add(resp_json, "cmd",
    //                        json_object_new_string("delete_group"));
    // json_object_object_add(resp_json, "group_id",
    //                        json_object_new_int(group_id));
    //
    // for (u32 i = 0; i < n_members; i++)
    // {
    //     member = gmembers + i;
    //     member_client = server_get_client_user_id(ew->server, member->user_id);
    //
    //     if (member_client)
    //         ws_json_send(member_client, resp_json);
    // }
    //
    // free(gmembers);
    //
    // return NULL;
}
