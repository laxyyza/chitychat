#include "server_websocket.h"
#include "server.h"
#include <sys/uio.h>
#include <json-c/json.h>

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

    }
    else
    {
        json_object* username_json = json_object_object_get(payload, "username");
        json_object* password_json = json_object_object_get(payload, "password");
        json_object* displayname_json = json_object_object_get(payload, "displayname");

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
            else
            {
                info("Logged in!!!\n");
            }
            // Find user by username from database
            // Compare the passwords

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