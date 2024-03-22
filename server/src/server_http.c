#include "server_http.h"
#include "server_util.h"
#include "server_log.h"
#include <fcntl.h>
#include "server.h"
#include "server_user_group.h"

#define NAME_CMP(x) !strncmp(header->name, x, HTTP_HEAD_NAME_LEN)

http_to_str_t 
http_to_str(const http_t* http)
{
    http_to_str_t to_str;
    size_t size;

#define HEADER_LINE_LEN (sizeof(http_header_t) + sizeof(HTTP_NL) + sizeof(": "))

    size = (HEADER_LINE_LEN * http->n_headers) + sizeof(HTTP_END);
    if (http->type == HTTP_REQUEST)
        size += sizeof(http_req_t);
    else
        size += sizeof(http_resp_t);

    if (http->body)
        size += http->body_len;
    
    to_str.max = size;
    to_str.str = calloc(1, size);


    if (http->type == HTTP_REQUEST)
        snprintf(to_str.str, size, "%s %s %s" HTTP_NL, http->req.method, http->req.url, http->req.version);
    else
        snprintf(to_str.str, size, "%s %u %s" HTTP_NL, http->resp.version, http->resp.code, http->resp.msg);

    for (size_t i = 0; i < http->n_headers; i++)
    {
        const http_header_t* header = &http->headers[i];
        if (header->name[0] == 0x00 || header->val[0] == 0x00)
            continue;

        char header_line[HEADER_LINE_LEN];
        snprintf(header_line, HEADER_LINE_LEN, "%s: %s%s", 
            header->name,
            header->val, 
            (i + 1 >= http->n_headers) ? "" : HTTP_NL // New line or not
        );
        strcat(to_str.str, header_line);
    }

    strcat(to_str.str, HTTP_END);
    size_t current_len = strnlen(to_str.str, to_str.max);
    if (http->body)
    {
        memcpy(to_str.str + current_len, http->body, http->body_len);
    }

    // to_str.len = strnlen(to_str.str, to_str.max);
    to_str.len = current_len + http->body_len;

    //printf("HTTP to string (%zu):\n%s\n================End.\n", to_str.len, to_str.str);

    return to_str;
}

static void 
set_client_connection(client_t* client, http_header_t* header)
{
    char* connection;
    char* saveptr;
    char* token;

    connection = header->val;
    token = strtok_r(connection, ", ", &saveptr);

    while (token)
    {
        if (!strncmp(token, "keep-alive", HTTP_HEAD_VAL_LEN))
            client->state |= CLIENT_STATE_KEEP_ALIVE;
        else if (!strncmp(token, "close", HTTP_HEAD_VAL_LEN))
            client->state = CLIENT_STATE_SHORT_LIVE;
        else if (!strncmp(token, "Upgrade", HTTP_HEAD_VAL_LEN))
            client->state |= CLIENT_STATE_UPGRADE_PENDING;
        else
            warn("HTTP Connection: '%s' not implemented.\n", token);

        token = strtok_r(NULL, ", ", &saveptr);
    }
}

static void 
handle_http_upgrade(client_t* client, http_header_t* header)
{
    if (client->state & CLIENT_STATE_UPGRADE_PENDING)
    {
        if (!strncmp(header->val, "websocket", HTTP_HEAD_VAL_LEN))
        {
            client->state |= CLIENT_STATE_WEBSOCKET;
        }
        else
        {
            warn("Connection upgrade '%s' not implemented!\n", header->val);
        }
    }
    else
    {
        warn("Client no upgrade connection?\n");
    }
}

static void 
handle_websocket_key(http_t* http, http_header_t* header)
{
    http->websocket_key = server_compute_websocket_key(header->val);
}

static void 
http_handle_content_len(http_t* http, http_header_t* header)
{
    char* endptr;
    http->body_len = strtoull(header->val, &endptr, 10);
    
    if ((errno == ERANGE && (http->body_len == ULONG_MAX)) || (errno != 0 && http->body_len == 0))
    {
        error("HTTP Content-Length: '%s' failed to convert to uint64_t: %s\n",
            header->val, ERRSTR);
    }

    if (endptr == header->val)
    {
        error("HTTP strtoull: No digits were found.\n");
    }
}

static void 
handle_http_header(client_t* client, http_t* http, http_header_t* header)
{
    if (NAME_CMP("Content-Length"))
    {
        http_handle_content_len(http, header);
    }
    else if (NAME_CMP("Connection"))
    {
        set_client_connection(client, header);
    }
    else if (NAME_CMP("Sec-WebSocket-Key"))
    {
        handle_websocket_key(http, header);
    }
}

static http_t* 
parse_http(client_t* client, char* buf, size_t buf_len) 
{
    http_t* http;
    char* saveptr;
    char* token;
    char* header;
    char* header_line;
    size_t header_len;
    size_t actual_body_len;

    http = calloc(1, sizeof(http_t));

    header = strsplit(buf, HTTP_END, &saveptr);
    if (!header)
    {
        warn("parse_http first token is NULL!?!\n");
        free(http);
        return NULL;
    }

    http->body = strsplit(NULL, HTTP_END, &saveptr);

    header_len = strlen(header) + strlen(HTTP_END);
    if (http->body)
        actual_body_len = buf_len - header_len;
    else
        actual_body_len = 0;

    header_line = strsplit(header, HTTP_NL, &saveptr);

    token = strtok(header_line, " ");
    if (token)
    {
        if (strstr(token, "HTTP/"))
        {
            http->type = HTTP_RESPOND;
            strncpy(http->resp.version, token, HTTP_VERSION_LEN);
        }
        else
        {
            http->type = HTTP_REQUEST;
            strncpy(http->req.method, token, HTTP_METHOD_LEN);
        }
    }
    else
    {
        free(http);
        return NULL;
    }

    token = strtok(NULL, " ");
    if (token)
    {
        if (http->type == HTTP_REQUEST)
            strncpy(http->req.url, token, HTTP_URL_LEN);
        else
            http->resp.code = atoi(token);
    }

    token = strtok(NULL, HTTP_NL);
    if (token)
    {
        if (http->type == HTTP_REQUEST)
            strncpy(http->req.version, token, HTTP_METHOD_LEN);
        else
            strncpy(http->resp.msg, token, HTTP_STATUS_MSG_LEN);
    }

    header_line = strsplit(NULL, HTTP_NL, &saveptr);

    while (header_line)
    {
        http_header_t* http_header = &http->headers[http->n_headers];
        char* name;
        char* val;

        token = strtok(header_line, ": ");
        if (!token)
            break;
        name = token;

        token = strtok(NULL, "");
        if (!token)
            break;
        if (*token == ' ')
            token++;
        val = token;

        strncpy(http_header->name, name, HTTP_HEAD_NAME_LEN);
        strncpy(http_header->val, val, HTTP_HEAD_VAL_LEN);

        header_line = strsplit(NULL, HTTP_NL, &saveptr);
        http->n_headers++;
        if (http->n_headers >= HTTP_MAX_HEADERS)
        {
            http->n_headers = HTTP_MAX_HEADERS - 1;
            break;
        }
    }

    for (size_t i = 0; i < http->n_headers; i++)
        handle_http_header(client, http, &http->headers[i]);

    if (client->state & CLIENT_STATE_UPGRADE_PENDING)
    {
        handle_http_upgrade(client, 
            http_get_header(
                http, 
                HTTP_HEAD_CONN_UPGRADE
            )
        );
    }

    if (http->body || http->body_len)
    {
        if (http->body_len > actual_body_len)
        {
            http->buf.missing = true;
            http->buf.total_recv = actual_body_len;
            char* new_body = calloc(1, http->body_len);
            memcpy(new_body, http->body, actual_body_len);
            http->body = new_body;
            http->body_inheap = true;
            client->recv.http = http;
        }
    }

    return http;
}

void 
print_parsed_http(const http_t* http)
{
    if (server_get_loglevel() != SERVER_VERBOSE)
        return;
    
    verbose("Parsed HTTP: %s\n", (http->type == HTTP_REQUEST) ? "Request" : "Respond");
    if (http->type == HTTP_REQUEST)
        verbose("\tMethod: '%s'\n\t\t\tURL: '%s'\n\t\t\tVersion: '%s'\n", http->req.method, http->req.url, http->req.version);
    else
        verbose("\tVersion: '%s'\n\t\t\tCode: %u\n\t\t\tStatus: '%s'\n", http->resp.version, http->resp.code, http->resp.msg);

    for (size_t i = 0; i < http->n_headers; i++)
    {
        const http_header_t* header = &http->headers[i];

        verbose("H:\t'%s' = '%s'\n", header->name, header->val);
    }

    if (http->body)
        verbose("BODY (strlen: %zu, http: %zu):\t'%s'\n", strlen(http->body), http->body_len, (http->body) ? http->body : "NULL");
}

static void 
http_free(http_t* http)
{
    if (!http)
        return;

    if (http->body && http->body_inheap)
        free(http->body);

    free(http);
}

http_header_t* 
http_get_header(const http_t* http, const char* name)
{
    for (size_t i = 0; i < http->n_headers; i++)
    {
        http_header_t* header = (http_header_t*)http->headers + i;

        if (NAME_CMP(name))
            return header;
    }

    return NULL;
}

static void 
http_add_header(http_t* http, const char* name, const char* val)
{
    if (!http || !name || !val)
        return;

    http_header_t* to_header = NULL;

    for (size_t i = 0; i < http->n_headers; i++)
    {
        http_header_t* header = http->headers + i;
        if (NAME_CMP(name) || header->name[0] == 0x00 || header->val[0] == 0x00)
        {
            to_header = header;
            break;
        }
    }

    if (!to_header)
    {
        if (http->n_headers >= HTTP_MAX_HEADERS)
        {
            warn("http_add_header(name: %s, val: %s): n headers is FULL!\n", name, val);
            return;
        }

        to_header = http->headers + http->n_headers;
        http->n_headers++;
    }

    strncpy(to_header->name, name, HTTP_HEAD_NAME_LEN);
    strncpy(to_header->val, val, HTTP_HEAD_VAL_LEN);
}

static void 
server_upgrade_client_to_websocket(client_t* client, http_t* req_http)
{
    http_t* http = http_new_resp(HTTP_CODE_SW_PROTO, "Switching Protocols", NULL, 0);
    http_add_header(http, "Connection", HTTP_HEAD_CONN_UPGRADE);
    http_add_header(http, "Upgrade", "websocket");
    http_add_header(http, HTTP_HEAD_WS_ACCEPT, req_http->websocket_key);

    if (http_send(client, http) != -1)
    {
        client->state |= CLIENT_STATE_WEBSOCKET;
        client->dbuser = calloc(1, sizeof(dbuser_t));
    }
    http_free(http);
    free(req_http->websocket_key);
}

static void 
server_handle_client_upgrade(client_t* client, http_t* http)
{
    const http_header_t* upgrade = http_get_header(http, HTTP_HEAD_CONN_UPGRADE);
    if (upgrade == NULL)
    {
        warn("Client fd:%d (Upgrade pending): No upgrade header in HTTP.\n", client->addr.sock);
        client->state ^= CLIENT_STATE_UPGRADE_PENDING;
        return;
    }

    if (!strncmp(upgrade->val, "websocket", HTTP_HEAD_VAL_LEN))
        server_upgrade_client_to_websocket(client, http);
    else
        warn("Connection upgrade '%s' not implemented.\n", upgrade);

    client->state ^= CLIENT_STATE_UPGRADE_PENDING;
}

static void 
http_add_body(http_t* restrict http, const char* restrict body, size_t body_len)
{
    http->body = calloc(1, body_len);
    memcpy(http->body, body, body_len);
    http->body_inheap = true;
    http->body_len = body_len;

    char val[HTTP_HEAD_VAL_LEN];
    snprintf(val, HTTP_HEAD_VAL_LEN, "%zu", body_len);

    http_add_header(http, HTTP_HEAD_CONTENT_LEN, val);
}

http_t* 
http_new_resp(u16 code, const char* status_msg, const char* body, size_t body_len)
{
    http_t* http = calloc(1, sizeof(http_t));

    http->type = HTTP_RESPOND;
    http->resp.code = code;
    strncpy(http->resp.msg, status_msg, HTTP_STATUS_MSG_LEN);
    strncpy(http->resp.version, HTTP_VERSION, HTTP_VERSION_LEN);

    if (body)
        http_add_body(http, body, body_len);

    return http;
}

static void 
server_http_resp_ok(client_t* client, char* content, size_t content_len, const char* content_type)
{
    http_t* http = http_new_resp(HTTP_CODE_OK, "OK", content, content_len);
    http_add_header(http, HTTP_HEAD_CONTENT_TYPE, content_type);

    http_send(client, http);

    http_free(http);
}

static void 
server_http_resp_error(client_t* client, u16 error_code, const char* status_msg)
{
    http_t* http = http_new_resp(error_code, status_msg, NULL, 0);

    http_send(client, http);

    http_free(http);
}

static void 
server_http_resp_404_not_found(client_t* client)
{
    const char* not_found_html = "<h1>Not Found</h1>";
    size_t len = strlen(not_found_html);

    http_t* http = http_new_resp(HTTP_CODE_NOT_FOUND, "Not Found", not_found_html, len);

    http_send(client, http);

    http_free(http);
}

static const char* 
server_get_content_type(const char* path)
{
#define CMP_EXT(ext) !strcmp(extention, ext)

    const char* extention = strrchr(path, '.');
    if (extention)
    {
        extention++;
        if (CMP_EXT("html"))
            return "text/html";
        else if (CMP_EXT("css"))
            return "text/css";
        else if (CMP_EXT("csv"))
            return "text/csv";
        else if (CMP_EXT("js"))
            return "application/javascript";
        else if (CMP_EXT("png"))
            return "image/png";
        else if (CMP_EXT("gif"))
            return "image/gif";
        else if (CMP_EXT("jpg") || CMP_EXT("jpeg"))
            return "image/jpeg";
        else if (CMP_EXT("bmp"))
            return "image/bmp";
        else if (CMP_EXT("svg"))
            return "image/svg+xml";
        else if (CMP_EXT("avif"))
            return "image/avif";
        else if (CMP_EXT("mp4"))
            return "video/mp4";
        else if (CMP_EXT("mov"))
            return "video/quicktime";
        else if (CMP_EXT("ico"))
            return "image/x-icon";
        else if (CMP_EXT("xml"))
            return "application/xml";
        else if (CMP_EXT("zip"))
            return "application/zip";
        else if (CMP_EXT("gz"))
            return "application/gzip";
        else if (CMP_EXT("json"))
            return "application/json";
    }

    return "application/octet-stream";
}

static int 
server_http_url_checks(http_t* http)
{
    const char* url = http->req.url;

    if (!http || !url)
        return -1;

    if (strstr(url, "../"))
    {
        warn("GET URL '%s' very sus.\n", url);
        return -1;
    }

    return 0;
}

static void 
server_handle_http_get(server_t* server, client_t* client, http_t* http)
{    
    char path[PATH_MAX];
    memset(path, 0, PATH_MAX);
    i32 fd;
    size_t content_len;
    char* content;
    size_t url_len = strnlen(http->req.url, HTTP_URL_LEN);

    if (server_http_url_checks(http) == -1)
    {
        server_http_resp_error(client, HTTP_CODE_NOT_FOUND, "Not found");
        return;
    }

    // TODO: Add "default_file_per_dir" config, default should be "index.html"
    if (http->req.url[url_len - 1] == '/')
        snprintf(path, PATH_MAX, "%s%s%s", server->conf.root_dir, http->req.url, "index.html"); 
    else
        snprintf(path, PATH_MAX, "%s%s", server->conf.root_dir, http->req.url); 
        //strncpy(path, http->req.url, HTTP_URL_LEN);
    
    i32 isdir = file_isdir(path);
    if (isdir == -1)
    {
        server_http_resp_404_not_found(client);
        return;
    }
    else if (isdir)
        strcat(path, "/index.html");

    const char* content_type = server_get_content_type(path);

    fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        error("GET request of '%s' failed: %s\n", path, ERRSTR);
        server_http_resp_404_not_found(client);
        return;
    }
    content_len = fdsize(fd);
    content = malloc(content_len);
    if (read(fd, content, content_len) == -1)
    {
        error("read '%s' failed: %s\n", path, ERRSTR);
        close(fd);
        server_http_resp_404_not_found(client);
        return;
    }
    close(fd);

    if (strcmp(content_type, "application/octet-stream") == 0)
    {
        const char* temp = server_mime_type(server, content, content_len);
        if (temp)
            content_type = temp;
    }

    verbose("Got file (%s): '%s'\n", content_type, path);
    server_http_resp_ok(client, content, content_len, content_type);
    free(content);
}

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
    // memset(real_token, 0, sizeof(upload_token_t));
    // server_del_upload_token(server, real_token);

    return real_token;
}

static void 
server_handle_user_pfp_update(server_t* server, client_t* client, const http_t* http, u32 user_id)
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

    user = server_db_get_user_from_id(server, user_id);
    if (!user)
    {
        warn("User %u not found.\n", user_id);
        failed = true;
        goto respond;
    }
    const char* filename = http->req.url + post_img_cmd_len;

    dbuser_file_t file;

    if (server_save_file_img(server, http->body, http->body_len, filename, &file))
    {
        if (!server_db_update_user(server, NULL, NULL, file.hash, user_id))
            failed = true;
        else
        {
            dbuser_file_t* dbfile = server_db_select_userfile(server, user->pfp_hash);
            if (dbfile)
            {
                server_delete_file(server, dbfile);
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
server_handle_msg_attach(server_t* server, client_t* client, const http_t* http, 
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

        info("Saving file '%s'...\n", name);
        
        if (server_save_file_img(server, http->body, http->body_len, name, &file))
        {
            json_object_object_add(attach_json, "hash",
                    json_object_new_string(file.hash));

            ut->msg_state.current++;
            if (ut->msg_state.current >= ut->msg_state.total)
            {
                msg->attachments = (char*)json_object_to_json_string(msg->attachments_json);

                if (server_db_insert_msg(server, msg))
                {
                    server_get_send_group_msg(server, msg->msg_id, msg->group_id);
                }
                server_del_upload_token(server, ut);
            }
        }
    }
    else
    {
        warn("Failed to get json array index: %zu\n", attach_index);
    }

respond:
    if (resp)
    {
        http_send(client, resp);
        http_free(resp);
    }
}

static void 
server_handle_http_post(server_t* server, client_t* client, const http_t* http)
{
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
        server_handle_user_pfp_update(server, client, http, user_id);
    }
    else if (ut->type == UT_MSG_ATTACHMENT)
    {
        server_handle_msg_attach(server, client, http, ut);
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

static void 
server_handle_http_req(server_t* server, client_t* client, http_t* http)
{
#define HTTP_CMP_METHOD(x) strncmp(http->req.method, x, HTTP_METHOD_LEN)

    if (!HTTP_CMP_METHOD("GET"))
        server_handle_http_get(server, client, http);
    else if (!HTTP_CMP_METHOD("POST"))
        server_handle_http_post(server, client, http);
    else
        warn("Need to implement '%s' HTTP request.\n", http->req.method);
}

static void 
server_handle_http_resp(UNUSED server_t* server, UNUSED client_t* client, UNUSED http_t* http)
{
    debug("Implement handling response http\n");
}

void 
server_handle_http(server_t* server, client_t* client, http_t* http)
{
    if (client->state & CLIENT_STATE_UPGRADE_PENDING)
        server_handle_client_upgrade(client, http);
    else
    {
        if (http->type == HTTP_REQUEST)
            server_handle_http_req(server, client, http);
        else if (http->type == HTTP_RESPOND)
            server_handle_http_resp(server, client, http);
        else
            warn("Unknown http type: %d. Request or Respond? Ignored.\n", http->type);
    }

    http_free(http);
}

enum client_recv_status 
server_http_parse(server_t* server, client_t* client, u8* buf, size_t buf_len)
{
    http_t* http;

    http = parse_http(client, (char*)buf, buf_len);
    if (!http)
    {
        error("parse_http() returned NULL, deleting client.\n");
        server_del_client(server, client);
        return RECV_DISCONNECT;
    }

    print_parsed_http(http);

    if (!http->buf.missing)
    {
        server_handle_http(server, client, http);
    }

    return RECV_OK;
}

ssize_t 
http_send(client_t* client, http_t* http)
{
    if (!client || !http)
    {
        warn("http_send(%p, %p) something is NULL\n", client, http);
        return -1;
    }

    ssize_t bytes_sent = 0;
    http_to_str_t to_str = http_to_str(http);

    verbose("HTTP send to fd:%d (%s:%s), len: %zu\n%s\n", client->addr.sock, client->addr.ip_str, client->addr.serv, to_str.len, to_str.str);

    if ((bytes_sent = server_send(client, to_str.str, to_str.len)) == -1)
    {
        error("HTTP send to (fd: %d, IP: %s:%s): %s\n",
            client->addr.sock, client->addr.ip_str, client->addr.serv, ERRSTR
        );
    }

    free(to_str.str);

    return bytes_sent;
}