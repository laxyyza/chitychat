#include "server.h"
#include "server_http.h"
#include "server_user_group.h"

static upload_token_t*
server_check_upload_token(server_t* server, const http_t* http, u32* user_id)
{
    char* endptr;
    const http_header_t* upload_token_header = http_get_header(http, "Upload-Token");
    const char* token_str = upload_token_header->val;
    if (!token_str)
    {
        error("No Upload-Token in POST request!\n");
        return NULL;
    }

    const u64 token = strtoull(token_str, &endptr, 10);
    if ((errno == ERANGE && (token == ULONG_MAX)) || (errno != 0 && token == 0))
    {
        error("HTTP POST Upload-Token: '%s' failed to convert to uint64_t: %s\n",
            token_str, ERRSTR);
        return NULL;
    }

    if (endptr == token_str)
    {
        error("HTTP POST strtoull: No digits were found.\n");
        return NULL;
    }

    upload_token_t* real_token = server_get_upload_token(server, token);
    if (real_token == NULL)
    {
        error("No upload token found: %zu\n", token);
        return NULL;
    }

    if (real_token->token != token)
    {
        error("*real_token != token?? %zu != %zu\n", *real_token, token);
        return NULL;
    }

    if (real_token->type == UT_USER_PFP && user_id)
        *user_id = real_token->user_id;

    return real_token;
}

static void 
server_handle_user_pfp_update(server_thread_t* th, client_t* client, const http_t* http, u32 user_id)
{
    http_t* resp = NULL;
    dbuser_t* user;
    bool failed = false;
    const char* post_img_cmd = "/img/";
    size_t post_img_cmd_len = strlen(post_img_cmd);

    if (strncmp(http->req.url, post_img_cmd, post_img_cmd_len) != 0)
    {
        resp = http_new_resp(HTTP_CODE_BAD_REQ, "Not image", NULL, 0);
        goto respond;
    }

    user = server_db_get_user_from_id(&th->db, user_id);
    if (!user)
    {
        warn("User %u not found.\n", user_id);
        failed = true;
        goto respond;
    }
    const char* filename = http->req.url + post_img_cmd_len;

    dbuser_file_t file;

    if (server_save_file_img(th, http->body, http->body_len, filename, &file))
    {
        if (!server_db_update_user(&th->db, NULL, NULL, file.hash, user_id))
            failed = true;
        else
        {
            dbuser_file_t* dbfile = server_db_select_userfile(&th->db, user->pfp_hash);
            if (dbfile)
            {
                server_delete_file(th, dbfile);
                free(dbfile);
            }
            else
                warn("User:%d %s (%s) no pfp?\n", user->user_id, user->username, user->displayname);
        }
    }
    else
        failed = true;

    free(user);

respond:
    if (!resp)
    {
        if (failed)
            resp = http_new_resp(HTTP_CODE_INTERAL_ERROR, "Interal server error", NULL, 0);
        else
            resp = http_new_resp(HTTP_CODE_OK, "OK", http->body, http->body_len);
    }

    http_send(client, resp);
    http_free(resp);
}

static void 
server_handle_msg_attach(server_thread_t* th, client_t* client, const http_t* http, 
            upload_token_t* ut)
{
    http_t* resp = NULL;
    dbmsg_t* msg = &ut->msg_state.msg;
    size_t attach_index = 0;
    char* endptr;
    http_header_t* attach_index_header = http_get_header(http, "Attach-Index");
    json_object* attach_json;
    json_object* name_json;
    const char* name;

    if (attach_index_header == NULL)
    {
        resp = http_new_resp(HTTP_CODE_BAD_REQ, "No Attach-Index header", NULL, 0);
        goto respond;
    }

    attach_index = strtoul(attach_index_header->val, &endptr, 10);
    if (attach_index == ULONG_MAX)
    {
        warn("Invalid Attach-Index: %s = %s\n", 
                attach_index_header->name, attach_index_header->val);
        resp = http_new_resp(HTTP_CODE_BAD_REQ, "Invalid Attach-Index", NULL, 0);
        goto respond;
    }

    if (attach_index > ut->msg_state.total)
    {
        warn("Attach-Index > msg_state.total. %zu/%zu\n", 
                attach_index, ut->msg_state.total);
    }

    attach_json = json_object_array_get_idx(msg->attachments_json, attach_index);
    if (attach_json)
    {
        name_json = json_object_object_get(attach_json, "name");
        name = json_object_get_string(name_json);
        dbuser_file_t file;

        if (server_save_file_img(th, http->body, http->body_len, name, &file))
        {
            json_object_object_add(attach_json, "hash",
                    json_object_new_string(file.hash));

            ut->msg_state.current++;
            if (ut->msg_state.current >= ut->msg_state.total)
            {
                msg->attachments = (char*)json_object_to_json_string(msg->attachments_json);

                if (server_db_insert_msg(&th->db, msg))
                    server_get_send_group_msg(th, msg, msg->group_id);

                server_del_upload_token(th->server, ut);
            }
        }
    }
    else
        warn("Failed to get json array index: %zu\n", attach_index);

respond:
    if (resp)
    {
        http_send(client, resp);
        http_free(resp);
    }
}


void 
server_handle_http_post(server_thread_t* th, client_t* client, const http_t* http)
{
    server_t* server = th->server;
    http_t* resp = NULL;
    u32 user_id;
    upload_token_t* ut = NULL;

    ut = server_check_upload_token(server, http, &user_id);

    if (ut == NULL)
    {
        resp = http_new_resp(HTTP_CODE_BAD_REQ, "Upload-Token failed", NULL, 0);
        goto respond;
    }

    if (ut->type == UT_USER_PFP)
    {
        free(ut);
        server_handle_user_pfp_update(th, client, http, user_id);
    }
    else if (ut->type == UT_MSG_ATTACHMENT)
    {
        server_handle_msg_attach(th, client, http, ut);
    }
    else
    {
        warn("Upload Token unknown type: %d\n", ut->type);
        free(ut);
    }

respond:
    if (resp)
    {
        http_send(client, resp);
        http_free(resp);
    }
}
