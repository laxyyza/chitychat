#include "server_http.h"
#include "server.h"
#include "server_log.h"
#include "server_auth.h"

// Vite server
#define DEV_ORIGIN_URI "http://localhost:5173"

#define NAME_CMP(x) !strncmp(header->name, x, HTTP_HEAD_NAME_LEN)
#define HEADER_LINE_LEN (sizeof(http_header_t) + sizeof(HTTP_NL) + sizeof(": "))

http_to_str_t 
http_to_str(const http_t* http)
{
    http_to_str_t to_str;
    size_t size;

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
        memcpy(to_str.str + current_len, http->body, http->body_len);

    to_str.len = current_len + http->body_len;

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

static inline void 
handle_get_cookie(http_t* http, char* cookie_keyval)
{
    char* saveptr;
    char* cookie_key = strtok_r(cookie_keyval, "=", &saveptr);
    char* cookie_val = strtok_r(NULL, "=", &saveptr);

    if (cookie_key == NULL || cookie_val == NULL)
        return;

    if (strcmp(cookie_key, "session") == 0)
        http->cookies.session_uuid = cookie_val;
    else if (strcmp(cookie_key, "remember_token") == 0)
        http->cookies.remember_token = cookie_val;
    else
        debug("Unknown cookie: %s=%s\n", cookie_key, cookie_val);
}

static inline void 
handle_cookie(http_t* http, http_header_t* header)
{
    char* saveptr;
    char* token_keyval = strtok_r(header->val, "; ", &saveptr);

    while (token_keyval)
    {
        handle_get_cookie(http, token_keyval);
        token_keyval = strtok_r(NULL, "; ", &saveptr);
    }
}

static inline void 
handle_origin(client_t* client, http_t* http, http_header_t* header)
{
    http->origin = header->val;
    client->dev_origin = strcmp(http->origin, "http://localhost:5173") == 0;

}

static inline void 
handle_user_agent(client_t* client, http_header_t* header)
{
    strncpy(client->user_agent, header->val, USER_AGENT_LEN);
}

static void 
handle_http_header(client_t* client, http_t* http, http_header_t* header)
{
    if (NAME_CMP("Content-Length"))
        http_handle_content_len(http, header);
    else if (NAME_CMP("Connection"))
        set_client_connection(client, header);
    else if (NAME_CMP("Sec-WebSocket-Key"))
        handle_websocket_key(http, header);
    else if (NAME_CMP("Cookie"))
        handle_cookie(http, header);
    else if (NAME_CMP("Origin"))
        handle_origin(client, http, header);
    else if (NAME_CMP("User-Agent"))
        handle_user_agent(client, header);
}

static void 
parse_url(http_t* http, char* url)
{
    char* path;
    char* params_line;
    char* param;
    char* endptr;

    path = strtok_r(url, "?", &params_line);

    if (params_line && *params_line)
    {
        param = strtok_r(params_line, "&", &endptr);

        while (param && http->n_params < HTTP_MAX_PARAMS)
        {
            char* key;
            char* val;
            http_header_t* http_param = http->params + http->n_params;

            key = strtok_r(param, "=", &val);

            if (key == NULL)
                break;
            else if (val == NULL)
                val = "";

            strncpy(http_param->name, key, HTTP_HEAD_NAME_LEN - 1);
            strncpy(http_param->val, val, HTTP_HEAD_VAL_LEN - 1);

            param = strtok_r(NULL, "&", &endptr);
            http->n_params++;
        }
    }

    strncpy(http->req.url, path, HTTP_URL_LEN - 1);
}

static http_t* 
parse_http(client_t* client, char* buf, size_t buf_len) 
{
    http_t* http;
    char* saveptr;
    char* token;
    char* header;
    char* header_line;
    char* strtok_saveptr;
    size_t header_len;
    size_t actual_body_len;

    http = calloc(1, sizeof(http_t));

    header = strsplit(buf, HTTP_END, &saveptr);
    if (!header)
    {
        warn("parse_http first token is NULL!?!\n");
        goto parse_error;
    }

    http->body = strsplit(NULL, HTTP_END, &saveptr);

    header_len = strlen(header) + strlen(HTTP_END);
    if (http->body)
        actual_body_len = buf_len - header_len;
    else
        actual_body_len = 0;

    header_line = strsplit(header, HTTP_NL, &saveptr);

    // Method
    token = strtok_r(header_line, " ", &strtok_saveptr);
    if (token)
    {
        http->type = HTTP_REQUEST;
        strncpy(http->req.method, token, HTTP_METHOD_LEN - 1);
    }
    else
    {
        warn("No token for method.\n");
        goto parse_error;
    }

    // URL
    token = strtok_r(NULL, " ", &strtok_saveptr);
    if (token)
    {
        if (http->type == HTTP_REQUEST)
            parse_url(http, token);
        else
            http->resp.code = atoi(token);
    }
    else 
    {
        warn("No URL\n");
        goto parse_error;
    }

    // HTTP Version
    token = strtok_r(NULL, HTTP_NL, &strtok_saveptr);
    if (token)
    {
        if (http->type == HTTP_REQUEST)
        {
            if (strcmp(token, "HTTP/1.1") != 0)
            {
                warn("Unsupported version: %s\n", token);
                server_http_resp(client, HTTP_CODE_VERSION_NOT_SUPP);
                goto parse_error;
            }
            strncpy(http->req.version, token, HTTP_METHOD_LEN - 1);
        }
        else
            strncpy(http->resp.msg, token, HTTP_STATUS_MSG_LEN - 1);
    }
    else
    {
        warn("No HTTP version included.\n");
        goto parse_error;
    }

    // Start of header
    header_line = strsplit(NULL, HTTP_NL, &saveptr);

    while (header_line)
    {
        http_header_t* http_header = &http->headers[http->n_headers];
        char* name;
        char* val;

        token = strtok_r(header_line, ": ", &strtok_saveptr);
        if (!token)
            break;
        name = token;

        token = strtok_r(NULL, "", &strtok_saveptr);
        if (!token)
            break;
        if (*token == ' ')
            token++;
        val = token;

        strncpy(http_header->name, name, HTTP_HEAD_NAME_LEN - 1);
        strncpy(http_header->val, val, HTTP_HEAD_VAL_LEN - 1);

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
parse_error:
    free(http);
    return NULL;
}

static void 
print_parsed_http(const http_t* http, client_t* client)
{
    info("HTTP Request: IP=[%s], Agent='%s', Method=%s, Path='%s'\n",
         client->addr.ip_str, client->user_agent, http->req.method, http->req.url);

    if (server_get_loglevel() != SERVER_VERBOSE)
        return;
    
    verbose("Parsed HTTP: %s\n", (http->type == HTTP_REQUEST) ? "Request" : "Respond");
    if (http->type == HTTP_REQUEST)
    {
        verbose("\tMethod: '%s'\n\t\t\tURL: '%s'\n\t\t\tVersion: '%s'\n", http->req.method, http->req.url, http->req.version);
        for (size_t i = 0; i < http->n_params; i++)
        {
            const http_header_t* param = http->params + i;
            verbose("\t'%s' = '%s'\n", param->name, param->val);
        }
    }
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

void 
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

char* 
http_add_header_adv(http_t* http, const char* name, const char* val, bool override)
{
    if (!http || !name)
        return NULL;

    http_header_t* to_header = NULL;

    if (override)
    {
        for (size_t i = 0; i < http->n_headers; i++)
        {
            http_header_t* header = http->headers + i;
            if (NAME_CMP(name) || header->name[0] == 0x00 || header->val[0] == 0x00)
            {
                to_header = header;
                break;
            }
        }
    }

    if (!to_header)
    {
        if (http->n_headers >= HTTP_MAX_HEADERS)
        {
            warn("http_add_header(name: %s, val: %s): n headers is FULL!\n", name, val);
            return NULL;
        }

        to_header = http->headers + http->n_headers;
        http->n_headers++;
    }

    strncpy(to_header->name, name, HTTP_HEAD_NAME_LEN - 1);
    if (val)
        strncpy(to_header->val, val, HTTP_HEAD_VAL_LEN - 1);

    return to_header->val;
}

char*
http_add_header(http_t* http, const char* name, const char* val)
{
    return http_add_header_adv(http, name, val, true);
}

void 
server_http_switch_to_websocket(client_t* client)
{
    http_t* http;

    http = http_new_resp(HTTP_CODE_SW_PROTO, NULL, 0);
    http_add_header(http, "Connection", HTTP_HEAD_CONN_UPGRADE);
    http_add_header(http, "Upgrade", "websocket");
    http_add_header(http, HTTP_HEAD_WS_ACCEPT, client->websocket_key);

    if (http_send(client, http) != -1)
        client->state |= CLIENT_STATE_WEBSOCKET;
    http_free(http);
    free(client->websocket_key);
    client->websocket_key = NULL;
}

static enum client_recv_status
server_upgrade_client_to_websocket(eworker_t* ew, client_t* client, http_t* http)
{
    client->websocket_key = http->websocket_key;

    return server_auth_websocket_upgrade(ew, client, http);
}

static enum client_recv_status
server_handle_client_upgrade(eworker_t* ew, client_t* client, http_t* http)
{
    enum client_recv_status ret = RECV_OK;

    const http_header_t* upgrade = http_get_header(http, HTTP_HEAD_CONN_UPGRADE);
    if (upgrade == NULL)
    {
        warn("Client fd:%d (Upgrade pending): No upgrade header in HTTP.\n", client->addr.sock);
        client->state ^= CLIENT_STATE_UPGRADE_PENDING;
        return RECV_ERROR;
    }

    if (!strncmp(upgrade->val, "websocket", HTTP_HEAD_VAL_LEN))
        ret = server_upgrade_client_to_websocket(ew, client, http);
    else
        warn("Connection upgrade '%s' not implemented.\n", upgrade);

    client->state ^= CLIENT_STATE_UPGRADE_PENDING;

    return ret;
}

static void 
http_add_body(http_t* restrict http, const char* restrict body, size_t body_len)
{
    http->body = calloc(1, body_len);
    memcpy(http->body, body, body_len);
    http->body_inheap = true;
    http->body_len = body_len;
}

http_t* 
http_new_resp(u16 code, const char* body, size_t body_len)
{
    http_t* http = calloc(1, sizeof(http_t));
    const char* status_msg = http_status_code_str(code);

    http->type = HTTP_RESPOND;
    http->resp.code = code;
    strncpy(http->resp.msg, status_msg, HTTP_STATUS_MSG_LEN - 1);
    strncpy(http->resp.version, HTTP_VERSION, HTTP_VERSION_LEN - 1);

    http_add_header(http, "Server", SERVER_NAME);

    if (body)
        http_add_body(http, body, body_len);

    char val[HTTP_HEAD_VAL_LEN];
    snprintf(val, HTTP_HEAD_VAL_LEN, "%zu", body_len);

    http_add_header(http, HTTP_HEAD_CONTENT_LEN, val);

    return http;
}

void 
http_add_cross_origin_headers(client_t* client, http_t* http)
{
    if (client->dev_origin)
    {
        http_add_header(http, "Access-Control-Allow-Credentials", "true");
        http_add_header(http, "Access-Control-Allow-Origin", DEV_ORIGIN_URI);
    }
}

void 
server_http_resp_ok(client_t* client, char* content, size_t content_len, const char* content_type)
{
    http_t* http = http_new_resp(HTTP_CODE_OK, content, content_len);
    http_add_header(http, HTTP_HEAD_CONTENT_TYPE, content_type);
    http_add_cross_origin_headers(client, http);

    http_send(client, http);

    http_free(http);
}

void 
server_http_resp(client_t* client, u16 error_code)
{
    http_t* http = http_new_resp(error_code, NULL, 0);
    http_add_header(http, "Content-Length", "0");
    http_add_cross_origin_headers(client, http);

    http_send(client, http);

    http_free(http);
}

void 
server_http_resp_json_single(client_t* client, u16 code, const char* key, const char* val)
{
    u64 body_len;
    json_object* json_body = json_object_new_object();
    json_object_object_add(json_body, key, json_object_new_string(val));
    const char* body = json_object_to_json_string_length(json_body, JSON_C_TO_STRING_NOSLASHESCAPE, &body_len);

    http_t* http = http_new_resp(code, body, body_len);
    http_add_header(http, HTTP_HEAD_CONTENT_TYPE, "application/json");

    http_send(client, http);

    http_free(http);
    json_object_put(json_body);
}

void 
server_http_resp_error(client_t* client, u16 error_code, const char* error_msg)
{
    server_http_resp_json_single(client, error_code, "error", error_msg);
}

void 
server_http_resp_404_not_found(client_t* client)
{
    const char* not_found_html = "<h1>Not Found</h1>";
    size_t len = strlen(not_found_html);

    http_t* http = http_new_resp(HTTP_CODE_NOT_FOUND, not_found_html, len);

    http_send(client, http);

    http_free(http);
}

int 
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

static enum client_recv_status
server_handle_http_options(client_t* client, http_t* http)
{
    http_t* resp = http_new_resp(HTTP_CODE_NO_CONTENT, NULL, 0);
    const http_header_t* req_headers = http_get_header(http, "Access-Control-Request-Headers");
    if (req_headers)
        http_add_header(resp, "Access-Control-Allow-Headers", req_headers->val);
    http_add_header(resp, "Access-Control-Allow-Methods", "*");
    http_add_header(resp, "Access-Control-Allow-Origin", DEV_ORIGIN_URI);
    http_add_header(resp, "Access-Control-Allow-Credentials", "true");

    http_send(client, resp);

    return RECV_OK;
}

static enum client_recv_status 
server_handle_http_req(eworker_t* th, client_t* client, http_t* http)
{
    enum client_recv_status ret = RECV_OK;

    if (!HTTP_CMP_METHOD("OPTIONS")) 
        return server_handle_http_options(client, http);

    if (str_startwith(http->req.url, "/api/"))
    {
        if (str_startwith(http->req.url, "/api/auth/"))
            return server_handle_auth(th, client, http);
        if (backend_route(th, client, http))
            return RECV_OK;
    }

    if (!HTTP_CMP_METHOD("GET") || !HTTP_CMP_METHOD("HEAD"))
        ret = server_handle_http_get(th->server, client, http);
    else if (!HTTP_CMP_METHOD("POST"))
        ret = server_handle_http_post(th, client, http);
    else
    {
        warn("Need to implement '%s' HTTP request.\n", http->req.method);
        ret = RECV_DISCONNECT;
    }

    return ret;
}

static enum client_recv_status
server_handle_http_resp(UNUSED server_t* server, UNUSED client_t* client, UNUSED http_t* http)
{
    debug("Implement handling response http\n");
    return RECV_ERROR;
}

enum client_recv_status
server_handle_http(eworker_t* ew, client_t* client, http_t* http)
{
    enum client_recv_status ret = RECV_OK;

    if (client->state & CLIENT_STATE_UPGRADE_PENDING)
        server_handle_client_upgrade(ew, client, http);
    else
    {
        if (http->type == HTTP_REQUEST)
            ret = server_handle_http_req(ew, client, http);
        else if (http->type == HTTP_RESPOND)
            ret = server_handle_http_resp(ew->server, client, http);
        else
        {
            warn("Unknown http type: %d. Request or Respond? Ignored.\n", http->type);
            ret = RECV_DISCONNECT;
        }
    }

    if (ew->ignore_http_free)
        ew->ignore_http_free = false;
    else
        http_free(http);

    return ret;
}

enum client_recv_status 
server_http_parse(eworker_t* th, client_t* client, u8* buf, size_t buf_len)
{
    http_t* http;
    enum client_recv_status ret = RECV_OK;

    http = parse_http(client, (char*)buf, buf_len);
    if (!http)
    {
        error("parse_http() returned NULL, deleting client.\n");
        return RECV_ERROR;
    }

    print_parsed_http(http, client);

    if (!http->buf.missing)
        ret = server_handle_http(th, client, http);

    return ret;
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

    verbose("HTTP SEND: IP=[%s], Agent='%s', Body:\n%s\n", 
            client->addr.ip_str, client->user_agent, to_str.str);

    if ((bytes_sent = server_send(client, to_str.str, to_str.len)) == -1)
    {
        error("HTTP send to (fd: %d, IP: %s:%s): %s\n",
            client->addr.sock, client->addr.ip_str, client->addr.serv, ERRSTR
        );
    }

    free(to_str.str);

    return bytes_sent;
}

const char* 
http_status_code_str(u32 code)
{
    switch (code)
    {
        case HTTP_CODE_CONTINUE:
            return "Continue";
        case HTTP_CODE_SW_PROTO:
            return "Switching Protocols";
        case HTTP_CODE_EARLY_HINTS:
            return "Early Hints";
        case HTTP_CODE_OK:
            return "OK";
        case HTTP_CODE_CREATED:
            return "Created";
        case HTTP_CODE_ACCEPTED:
            return "Accepted";
        case HTTP_CODE_NO_CONTENT:
            return "No Content";
        case HTTP_CODE_RESET_CONTENT:
            return "Reset Content";
        case HTTP_CODE_PARTIAL_CONTENT:
            return "Partial Content";
        case HTTP_CODE_IM_USED:
            return "IM Used";
        case HTTP_CODE_BAD_REQ:
            return "Bad Request";
        case HTTP_CODE_UNAUTHORIZED:
            return "Unauthorized";
        case HTTP_CODE_FORBIDDEN:
            return "Forbidden";
        case HTTP_CODE_NOT_FOUND:
            return "Not Found";
        case HTTP_CODE_METH_NOT_ALLOW:
            return "Method Not Allowed";
        case HTTP_CODE_NOT_ACCEPTABLE:
            return "Not Acceptable";
        case HTTP_CODE_REQ_TIMEOUT:
            return "Request Timeout";
        case HTTP_CODE_CONFLICT:
            return "Conflict";
        case HTTP_CODE_GONE:
            return "Gone";
        case HTTP_CODE_LEN_REQUIRED:
            return "Length Required";
        case HTTP_CODE_PRECON_FAILED:
            return "Precondition Failed";
        case HTTP_CODE_PAYLOAD_LARGE:
            return "Payload Too Large";
        case HTTP_CODE_URI_TOO_LONG:
            return "URI Too Long";
        case HTTP_CODE_UNSUPP_MEDIA:
            return "Unsupported Media Type";
        case HTTP_CODE_RANGE_NOT_SAT:
            return "Range Not Satisfiable";
        case HTTP_CODE_EXPECTATION_F:
            return "Expectation Failed";
        case HTTP_CODE_MISDIRECTED_R:
            return "Misdirected Request";
        case HTTP_CODE_UNPROCESS_CONTENT:
            return "Unprocessable Content";
        case HTTP_CODE_TOO_EARLY:
            return "Too Early";
        case HTTP_CODE_UPGRADE_REQUIRED:
            return "Upgrade Required";
        case HTTP_CODE_PRECOND_REQUIRED:
            return "Precondition Required";
        case HTTP_CODE_TOO_MANY_REQUESTS:
            return "Too Many Requests";
        case HTTP_CODE_HEADERS_TO_LARGE:
            return "Request Header Fields Too Large";
        case HTTP_CODE_INTERAL_ERROR:
            return "Internal Server Error";
        case HTTP_CODE_NOT_IMPLEMENT:
            return "Not Implemented";
        case HTTP_CODE_BAD_GATEWAY:
            return "Bad Gateway";
        case HTTP_CODE_SERV_UNAVAIL:
            return "Service Unavailable";
        case HTTP_CODE_GATEWAY_TIMEOUT:
            return "Gateway Timeout";
        case HTTP_CODE_VERSION_NOT_SUPP:
            return "HTTP Version Not Supported";
        case HTTP_CODE_VARIANT_ALSO_NEGO:
            return "Variant Also Negotiates";
        case HTTP_CODE_NOT_EXTENDED:
            return "Not Extended";
        case HTTP_CODE_NET_AUTH_REQUIRED:
            return "Network Authentication Required";
        default:
            return "";
    }
}
