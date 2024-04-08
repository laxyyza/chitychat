#include "server_user_group.h"
#include "server.h"
#include "server_client.h"
#include "server_db.h"
#include "server_tm.h"
#include <json-c/json_object.h>

static void
server_msg_to_json(const dbmsg_t* dbmsg, json_object* json)
{
    json_object* attachments_json;

    attachments_json = json_tokener_parse(dbmsg->attachments);

    json_object_object_add(json, "type", 
            json_object_new_string("group_msg"));
    json_object_object_add(json, "msg_id", 
            json_object_new_int(dbmsg->msg_id));
    json_object_object_add(json, "group_id", 
            json_object_new_int(dbmsg->group_id));
    json_object_object_add(json, "user_id", 
            json_object_new_int(dbmsg->user_id));
    json_object_object_add(json, "content", 
            json_object_new_string(dbmsg->content));
    json_object_object_add(json, "attachments",
                attachments_json);
    json_object_object_add(json, "timestamp", 
            json_object_new_string(dbmsg->timestamp));
}

static void 
server_group_to_json(server_thread_t* th, const dbgroup_t* dbgroup, json_object* json)
{
    u32 n_members;
    dbuser_t* gmembers;
    json_object* members_array_json;

    json_object_object_add(json, "id", 
                           json_object_new_int(dbgroup->group_id));
    json_object_object_add(json, "owner_id",
                           json_object_new_int(dbgroup->owner_id));
    json_object_object_add(json, "name",
                           json_object_new_string(dbgroup->displayname));
    json_object_object_add(json, "desc",
                           json_object_new_string(dbgroup->desc));
    json_object_object_add(json, "public",
                           json_object_new_boolean(dbgroup->flags & DB_GROUP_PUBLIC));

    if (th)
    {
        gmembers = server_db_get_group_members(&th->db, dbgroup->group_id, &n_members);
        
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

const char* 
server_group_create(server_thread_t* th, client_t* client, json_object* payload, 
                    json_object* respond_json)
{
    bool public_group = false;
    json_object* name_json = json_object_object_get(payload, "name");
    const char* name = json_object_get_string(name_json);
    json_object* public_json = json_object_object_get(payload, "public");
    if (public_json)
        public_group = json_object_get_boolean(public_json);

    dbgroup_t new_group;
    memset(&new_group, 0, sizeof(dbgroup_t));
    new_group.owner_id = client->dbuser->user_id;
    if (public_group)
        new_group.flags |= DB_GROUP_PUBLIC;
    strncpy(new_group.displayname, name, DB_DISPLAYNAME_MAX);

    if (!server_db_insert_group(&th->db, &new_group))
    {
        return "Failed to create group";
    }

    info("Creating new group, id: %u, name: '%s', owner_id: %u\n", 
                new_group.group_id, name, new_group.owner_id);

    json_object_object_add(respond_json, "type", 
            json_object_new_string("client_groups"));
    json_object_object_add(respond_json, "groups", 
            json_object_new_array_ext(1));
    json_object* group_array_json = json_object_object_get(respond_json, 
                "groups");

    dbgroup_t* g = server_db_get_group(&th->db, new_group.group_id);
    json_object* group_json = json_object_new_object();
    json_object_object_add(group_json, "id", 
            json_object_new_int(g->group_id));
    json_object_object_add(group_json, "name", 
            json_object_new_string(g->displayname));
    json_object_object_add(group_json, "desc", 
            json_object_new_string(g->desc));

    u32 n_members;
    dbuser_t* gmembers = server_db_get_group_members(&th->db, 
                                                     g->group_id, &n_members);
    
    json_object_object_add(group_json, "members_id",    
            json_object_new_array_ext(n_members));
    json_object* members_array_json = json_object_object_get(group_json, 
            "members_id");

    // TODO: Add full user info in packet
    for (u32 m = 0; m < n_members; m++)
    {
        const dbuser_t* member = gmembers + m;
        json_object_array_add(members_array_json, 
                json_object_new_int(member->user_id));
    }

    json_object_array_add(group_array_json, group_json);

    ws_json_send(client, respond_json);
    
    free(g);
    free(gmembers);

    return NULL;
}

const char* 
server_client_groups(server_thread_t* th, client_t* client, json_object* respond_json) 
{
    dbgroup_t* groups;
    u32 n_groups;

    groups = server_db_get_user_groups(&th->db, client->dbuser->user_id, 
            &n_groups);

    if (!groups)
        return "Server failed to get user groups";

    json_object_object_add(respond_json, "type", 
            json_object_new_string("client_groups"));
    json_object_object_add(respond_json, "groups", 
                json_object_new_array_ext(n_groups));
    json_object* group_array_json = json_object_object_get(respond_json, 
                "groups");

    for (size_t i = 0; i < n_groups; i++)
    {
        const dbgroup_t* g = groups + i;
        json_object* group_json = json_object_new_object();

        server_group_to_json(th, g, group_json);
        json_object_array_add(group_array_json, group_json);
    }

    ws_json_send(client, respond_json);

    free(groups);

    return NULL;
}

static bool
server_client_in_group(server_thread_t* th, const client_t* client, const dbgroup_t* group)
{
    bool ret = false;
    dbuser_t* gmembers;
    u32     n_members;

    if (!client || !client->dbuser || !group || !th)
        return false;

    gmembers = server_db_get_group_members(&th->db, group->group_id, &n_members);
    if (gmembers)
    {
        for (u32 i = 0; i < n_members; i++)
        {
            dbuser_t* member = gmembers + i;
            if (member->user_id == client->dbuser->user_id)
            {
                ret = true;
                break;
            }
        }

        free(gmembers);
    }

    return ret;

}

const char* 
server_get_all_groups(server_thread_t* th, client_t* client, json_object* respond_json)
{
    u32 n_groups;
    dbgroup_t* groups = server_db_get_all_groups(&th->db, &n_groups);
    if (!groups)
        return "Failed to get groups";

    json_object_object_add(respond_json, "type", 
            json_object_new_string("get_all_groups"));
    json_object_object_add(respond_json, "groups", 
            json_object_new_array_ext(n_groups));
    json_object* groups_json = json_object_object_get(respond_json, 
            "groups");

    for (size_t i = 0; i < n_groups; i++)
    {
        dbgroup_t* g = groups + i;
        if (g->flags & DB_GROUP_PUBLIC || server_client_in_group(th, client, g))
        {
            json_object* group_json = json_object_new_object();

            server_group_to_json(NULL, g, group_json);
            json_object_array_add(groups_json, group_json);
        }
    }

    ws_json_send(client, respond_json);

    free(groups);

    return NULL;
}

static void
on_user_group_join(server_thread_t* th, client_t* client,
                   dbgroup_t* group, json_object* respond_json)
{
    json_object_object_add(respond_json, "type", 
            json_object_new_string("client_groups"));
    json_object_object_add(respond_json, "groups", 
            json_object_new_array_ext(1));
    json_object* array_json = json_object_object_get(respond_json, 
            "groups");

    // TODO: DRY!
    json_object* group_json = json_object_new_object();
    json_object_object_add(group_json, "id",            // TODO: Make "id" to "group_id"
            json_object_new_int(group->group_id));
    json_object_object_add(group_json, "name", 
            json_object_new_string(group->displayname));
    json_object_object_add(group_json, "desc", 
            json_object_new_string(group->desc));

    u32 n_members;
    dbuser_t* gmembers = server_db_get_group_members(&th->db, 
                                                     group->group_id, &n_members);

    json_object_object_add(group_json, "members_id", 
            json_object_new_array_ext(n_members));
    json_object* members_array_json = json_object_object_get(group_json, 
            "members_id");

    json_object* other_clients_respond = json_object_new_object();
    json_object_object_add(other_clients_respond, "type", 
        json_object_new_string("join_group"));
    json_object_object_add(other_clients_respond, "user_id", 
        json_object_new_int(client->dbuser->user_id));
    json_object_object_add(other_clients_respond, "group_id", 
        json_object_new_int(group->group_id));

    for (u32 m = 0; m < n_members; m++)
    {
        const dbuser_t* member = gmembers + m;
        const client_t* member_client = server_get_client_user_id(th->server, 
                member->user_id);

        if (member_client && member_client != client)
            ws_json_send(member_client, other_clients_respond);

        json_object_array_add(members_array_json, 
                json_object_new_int(member->user_id));
    }

    json_object_put(other_clients_respond);
    json_object_array_add(array_json, group_json);

    ws_json_send(client, respond_json);

    free(gmembers);
}

const char* 
server_join_group(server_thread_t* th, client_t* client, 
        json_object* payload, json_object* respond_json)
{
    json_object* group_id_json = json_object_object_get(payload, 
            "group_id");
    const u64 group_id = json_object_get_int(group_id_json);

    dbgroup_t* group = server_db_get_group(&th->db, group_id);
    if (!group)
        return "Group not found";

    if (!server_db_insert_group_member(&th->db, group->group_id, 
                                       client->dbuser->user_id))
        return "Failed to join";

    on_user_group_join(th, client, group, respond_json);

    free(group);

    return NULL;
}

const char*
server_get_send_group_msg(server_thread_t* th, 
                const dbmsg_t* dbmsg, const u32 group_id)
{
    json_object* respond_json;
    u32 n_memebrs;
    dbuser_t* gmemebrs;

    respond_json = json_object_new_object();

    gmemebrs = server_db_get_group_members(&th->db, group_id, 
                                                    &n_memebrs);
    if (!gmemebrs)
        return "Failed to get group members";

    server_msg_to_json(dbmsg, respond_json);

    // Update all other online group members
    for (size_t i = 0; i < n_memebrs; i++)
    {
        const dbuser_t* member = gmemebrs + i;
        const client_t* member_client = server_get_client_user_id(th->server, 
                member->user_id);

        // If member_client is not NULL it means they are online.
        if (member_client)
            ws_json_send(member_client, respond_json);
    }

    free(gmemebrs);

    json_object_put(respond_json);

    return NULL;
}

const char* 
server_group_msg(server_thread_t* th, client_t* client, 
            json_object* payload, UNUSED json_object* respond_json)
{
    json_object* group_id_json = json_object_object_get(payload, 
            "group_id");
    json_object* content_json = json_object_object_get(payload, 
            "content");
    json_object* attachments_json = json_object_object_get(payload, 
            "attachments");
    const u64 group_id = json_object_get_int(group_id_json);
    const char* content = json_object_get_string(content_json);
    const u64 user_id = client->dbuser->user_id;
    const size_t len = json_object_array_length(attachments_json);

    dbmsg_t new_msg = {
        .user_id = user_id,
        .group_id = group_id,
    };
    strncpy(new_msg.content, content, DB_MESSAGE_MAX);

    if (len == 0)
    {
        if (!server_db_insert_msg(&th->db, &new_msg))
            return "Failed to send message";

        return server_get_send_group_msg(th, &new_msg, group_id);
    }
    else
    {
        upload_token_t* ut = server_new_upload_token_attach(th->server, &new_msg);
        ut->msg_state.msg.attachments_json = json_object_get(attachments_json);
        ut->msg_state.total = len;

        server_send_upload_token(client, "send_attachments", ut);

        return NULL;
    }
}

const char* 
get_group_msgs(server_thread_t* th, client_t* client, json_object* payload, 
                    json_object* respond_json)
{
    json_object* limit_json;
    json_object* group_id_json;
    json_object* offset_json;
    u64 group_id;
    u32 limit;
    u32 offset;
    u32 n_msgs;
    dbmsg_t* msgs;

    group_id_json = json_object_object_get(payload, 
            "group_id");
    limit_json = json_object_object_get(payload, 
            "limit");
    offset_json = json_object_object_get(payload, "offset");

    group_id = json_object_get_int(group_id_json);
    limit = (limit_json) ? json_object_get_int(limit_json) : -1;
    offset = (offset_json) ? json_object_get_int(offset_json) : 0;

    msgs = server_db_get_msgs_from_group(&th->db, group_id, limit, 
            offset, &n_msgs);

    json_object_object_add(respond_json, "type", 
            json_object_new_string("get_group_msgs"));
    json_object_object_add(respond_json, "group_id", 
            json_object_new_int(group_id));
    json_object_object_add(respond_json, "messages", 
            json_object_new_array_ext(n_msgs));
    json_object* msgs_json = json_object_object_get(respond_json, "messages");

    for (u32 i = 0; i < n_msgs; i++)
    {
        dbmsg_t* msg = msgs + i;

        json_object* msg_in_array = json_object_new_object();

        server_msg_to_json(msg, msg_in_array);

        json_object_array_add(msgs_json, msg_in_array);

        if (msg->attachments_inheap && msg->attachments)
        {
            free(msg->attachments);
            msg->attachments_inheap = false;
        }
    }

    ws_json_send(client, respond_json);

    free(msgs);
    return NULL;
}

const char* server_create_group_code(server_thread_t* th, 
                                     client_t* client,
                                     json_object* payload, 
                                     json_object* respond_json)
{
    json_object* group_id_json;
    json_object* uses_json;
    dbgroup_code_t group_code;
    u32 group_id;
    i32 uses;

    group_id_json = json_object_object_get(payload, "group_id");
    group_id = json_object_get_int(group_id_json);

    uses_json = json_object_object_get(payload, "uses");
    if (uses_json)
        uses = json_object_get_int(uses_json);
    else 
        uses = -1;

    if (!client->dbuser || !server_db_user_in_group(&th->db, group_id, 
                                                    client->dbuser->user_id))
        return "Not a group member";

    group_code.group_id = group_id;
    group_code.uses = uses;

    if (!server_db_insert_group_code(&th->db, &group_code))
        return "Failed to create group code";

    json_object_object_add(respond_json, "type", 
                           json_object_new_string("create_group_code"));
    json_object_object_add(respond_json, "group_id", 
                           json_object_new_int(group_code.group_id));
    json_object_object_add(respond_json, "uses", 
                           json_object_new_int(group_code.uses));
    json_object_object_add(respond_json, "code", 
                           json_object_new_string_len(group_code.invite_code, 
                                                      DB_GROUP_CODE_MAX));

    ws_json_send(client, respond_json);

    return NULL;
}

const char* server_join_group_code(server_thread_t* th,
                                   client_t* client,
                                   json_object* payload,
                                   json_object* respond_json)
{
    json_object* code_json;
    const char* code;
    const char* errmsg = NULL;
    const u32 user_id = client->dbuser->user_id;
    dbgroup_t* group_joined;

    code_json = json_object_object_get(payload, "code");
    code = json_object_get_string(code_json);

    group_joined = server_db_insert_group_member_code(&th->db,
                                                      code,
                                                      user_id);
    if (!group_joined)
        return "Failed to join or already joined";

    on_user_group_join(th, client, group_joined, respond_json);
        
    free(group_joined);
    return errmsg;
}

const char* server_get_group_codes(server_thread_t* th,
                                   client_t* client,
                                   json_object* payload,
                                   json_object* respond_json)
{
    json_object* group_id_json;
    u32 group_id;
    dbgroup_code_t* codes;
    u32 n_codes;
    json_object* codes_array_json;

    group_id_json = json_object_object_get(payload, "group_id");
    group_id = json_object_get_int(group_id_json);

    if (!server_db_user_in_group(&th->db, group_id, client->dbuser->user_id))
        return "Not a group member";

    codes = server_db_get_all_group_codes(&th->db, group_id, &n_codes);

    json_object_object_add(respond_json, "type", 
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

    if (codes)
        free(codes);
    return NULL;
}
