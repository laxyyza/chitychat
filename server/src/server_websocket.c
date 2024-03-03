#include "server_websocket.h"
#include "server.h"
#include <sys/uio.h>
#include <json-c/json.h>
#include <sys/random.h>

static session_t* server_find_session(server_t* server, u32 id)
{
    if (id == 0)
        return NULL;

    for (size_t i = 0; i < MAX_SESSIONS; i++)
        if (server->sessions[i].session_id == id)
            return &server->sessions[i];

    return NULL;
}

ssize_t ws_json_send(client_t* client, json_object* json)
{
    size_t len;
    const char* string = json_object_to_json_string_length(json, 0, &len);

    return ws_send(client, string, len);
}

static void server_ws_handle_text_frame(server_t* server, client_t* client, char* buf, size_t buf_len)
{
    info("Web Socket message from fd:%d, IP: %s:%s:\n\t'%s'\n", 
        client->addr.sock, client->addr.ip_str, client->addr.serv, buf
    );

    json_object* respond_json = json_object_new_object();
    json_object* payload = json_tokener_parse(buf);
    json_object* type_json = json_object_object_get(payload, "type");
    const char* type = json_object_get_string(type_json);
    const char* error_msg = NULL;

    if (client->state & CLIENT_STATE_LOGGED_IN)
    {
        if (!strcmp(type, "client_user_info"))
        {
            json_object_object_add(respond_json, "type", json_object_new_string("client_user_info"));
            json_object_object_add(respond_json, "id", json_object_new_int(client->dbuser.user_id));
            json_object_object_add(respond_json, "username", json_object_new_string(client->dbuser.username));
            json_object_object_add(respond_json, "displayname", json_object_new_string(client->dbuser.displayname));
            json_object_object_add(respond_json, "bio", json_object_new_string(client->dbuser.bio));

            ws_json_send(client, respond_json);
        }
    }
    else
    {
        json_object* username_json = json_object_object_get(payload, "username");
        json_object* password_json = json_object_object_get(payload, "password");
        json_object* displayname_json = json_object_object_get(payload, "displayname");


        if (!strcmp(type, "session"))
        {
            json_object* session_id_json = json_object_object_get(payload, "id");
            const u32 session_id = json_object_get_int(session_id_json);

            const session_t* session = server_find_session(server, session_id);
            if (!session)
            {
                error_msg = "Invalid session ID or session expired";
                goto error;
            }

            dbuser_t* dbuser = server_db_get_user_from_id(server, session->user_id);
            if (!dbuser)
            {
                error_msg = "Server cuold not find user in database";
                goto error;
            }
            memcpy(&client->dbuser, dbuser, sizeof(dbuser_t));
            free(dbuser);

            json_object_object_add(respond_json, "type", json_object_new_string("session"));
            json_object_object_add(respond_json, "id", json_object_new_uint64(session->session_id));

            size_t len;
            const char* respond = json_object_to_json_string_length(respond_json, 0, &len);

            if (ws_send(client, respond, len) != -1)
                client->state |= CLIENT_STATE_LOGGED_IN;

            info("Client with session ID: %u, logged in as: %u\n", session->session_id, session->user_id);

            goto error;
        }

        const char* username = json_object_get_string(username_json);
        const char* password = json_object_get_string(password_json);
        const char* displayname = json_object_get_string(displayname_json);

        if (!username_json || !username)
        {
            error_msg = "Require username.";
            goto error;
        }
        if (!password_json || !password)
        {
            error_msg = "Require password.";
            goto error;
        }

        if (!strcmp(type, "login"))
        {
            dbuser_t* user = server_db_get_user_from_name(server, username);
            if (!user)
            {
                error_msg = "Incorrect Username or password";
                goto error;
            }

            info("Found user: %zu|%s|%s|%s|%s\n", user->user_id, user->username, user->displayname, user->bio, user->password);

            if (strncmp(password, user->password, DB_PASSWORD_MAX))
            {
                error_msg = "Incorrect Username or password";
                goto error;
            }

            info("Logged in!!!\n");

            free(user);
        }
        else if (!strcmp(type, "register"))
        {
            if (!displayname_json || !displayname)
            {
                error_msg = "Require display name.";
                goto error;
            }
            // Insert new user  

            dbuser_t new_user;
            memset(&new_user, 0, sizeof(dbuser_t));
            strncpy(new_user.username, username, DB_USERNAME_MAX);
            strncpy(new_user.displayname, displayname, DB_USERNAME_MAX);
            strncpy(new_user.password, password, DB_PASSWORD_MAX);

            if (!server_db_insert_user(server, &new_user))
            {
                error_msg = "Username already taken";
                goto error;
            }
        }
        else
        {
            error_msg = "Not logged in.";
            goto error;
            // NOPE
        }

        session_t* session;
        dbuser_t* user = server_db_get_user_from_name(server, username);
        for (size_t i = 0; i < MAX_SESSIONS; i++)
        {
            if (server->sessions[i].session_id == 0)
            {
                session = &server->sessions[i];
                break;
            }
        }

        getrandom(&session->session_id, sizeof(u32), 0);
        session->user_id = user->user_id;

        json_object_object_add(respond_json, "type", json_object_new_string("session"));
        json_object_object_add(respond_json, "id", json_object_new_uint64(session->session_id));

        size_t len;
        const char* respond = json_object_to_json_string_length(respond_json, 0, &len);

        info("New session created: %u\n", session->session_id);

        ws_send(client, respond, len);
        client->state |= CLIENT_STATE_LOGGED_IN;

        free(user);
    }


error:;
    if (error_msg)
    {
        error("error:msg: %s\n", error_msg);
        json_object_object_add(respond_json, "type", json_object_new_string("error"));
        json_object_object_add(respond_json, "error_msg", json_object_new_string(error_msg));

        size_t len;
        const char* respond = json_object_to_json_string_length(respond_json, 0, &len);

        ws_send(client, respond, len);
    }

    json_object_put(payload);
    json_object_put(respond_json);
}

void server_ws_parse(server_t* server, client_t* client, u8* buf, size_t buf_len)
{
    ws_t ws;
    memset(&ws, 0, sizeof(ws_t));
    size_t offset = sizeof(ws_frame_t);

    memcpy(&ws.frame, buf, sizeof(ws_frame_t));

    if (ws.frame.payload_len == 126)
    {
        ws.ext.u16 = WS_PAYLOAD_LEN16(buf);
        ws.payload_len = (u16)ws.ext.u16;
        offset += 2;
        debug("Payload len 16-bit: %zu\n", ws.payload_len);
    }
    else if (ws.frame.payload_len == 127)
    {
        ws.ext.u64 = WS_PAYLOAD_LEN64(buf);
        ws.payload_len = ws.ext.u64;
        offset += 8;
        debug("Payload len 64-bit\n");
    }
    else
        ws.payload_len   = ws.frame.payload_len;

    if (ws.frame.mask)
    {
        memcpy(ws.ext.maskkey, buf + offset, WS_MASKKEY_LEN);
        offset += WS_MASKKEY_LEN;
        ws.payload = (char*)&buf[offset];
        mask((u8*)ws.payload, ws.payload_len, ws.ext.maskkey, WS_MASKKEY_LEN);
    }
    else
        ws.payload = (char*)buf + offset;

    switch (ws.frame.opcode)
    {
        case WS_CONTINUE_FRAME:
            debug("CONTINUE FRAME: %s\n", ws.payload);
            break;
        case WS_TEXT_FRAME:
            server_ws_handle_text_frame(server, client, ws.payload, ws.payload_len);
            break;
        case WS_BINARY_FRAME:
            debug("BINARY FRAME: %s\n", ws.payload);
            break;
        case WS_CLOSE_FRAME:
        {
            debug("CLOSE FRAME: %s\n", ws.payload);
            server_del_client(server, client);
            break;
        }
        case WS_PING_FRAME:
            debug("PING FRAME: %s\n", ws.payload);
            break;
        case WS_PONG_FRAME:
            debug("PONG FRAME: %s\n", ws.payload);
            break;
        default:
            debug("UNKNOWN FRAME (%u): %s\n", ws.frame.opcode, ws.payload);
            break;
    }
}

ssize_t ws_send_adv(client_t* client, u8 opcode, const char* buf, size_t len, const u8* maskkey)
{
    ssize_t bytes_sent = 0;
    struct iovec iov[4];
    size_t i = 0;
    ws_t ws = {
        .frame.fin = 1,
        .frame.rsv1 = 0,
        .frame.rsv2 = 0,
        .frame.rsv3 = 0,
        .frame.opcode = opcode,
        .frame.mask = (maskkey) ? 1 : 0,
        .frame.payload_len = len
    };

    iov[i].iov_base = &ws.frame;
    iov[i].iov_len = sizeof(ws_frame_t);

    if (len >= 125)
    {
        i++;
        if (len >= UINT16_MAX)
        {
            debug("WS SEND 64-bit\n");
            ws.frame.payload_len = 127;
            swpcpy((u8*)&ws.ext.u64, (u8*)&len, sizeof(u64));
            iov[i].iov_base = &ws.ext.u64;
            iov[i].iov_len = sizeof(u64);
        }
        else
        {
            debug("WS SEND 16-bit\n");
            ws.frame.payload_len = 126;
            swpcpy((u8*)&ws.ext.u16, (const u8*)&len, sizeof(u16));
            iov[i].iov_base = &ws.ext.u16;
            iov[i].iov_len = sizeof(u16);
        }
    }

    if (ws.frame.mask)
    {
        i++;
        iov[i].iov_base = (u8*)maskkey;
        iov[i].iov_len = WS_MASKKEY_LEN;

        if (buf)
            mask((u8*)buf, len, maskkey, WS_MASKKEY_LEN);

    }

    if (buf)
    {
        i++;
        iov[i].iov_base = (char*)buf;
        iov[i].iov_len = len;
    }

    i++;
    if ((bytes_sent = writev(client->addr.sock, iov, i)) == -1)
    {
        error("WS Send to (fd:%d, IP: %s:%s): %s\n",
            client->addr.sock, client->addr.ip_str, client->addr.serv
        );
    }

    return bytes_sent;
}

ssize_t ws_send(client_t* client, const char* buf, size_t len)
{
    return ws_send_adv(client, WS_TEXT_FRAME, buf, len, NULL);
}