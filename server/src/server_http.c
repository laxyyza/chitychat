#include "server_http.h"
#include "server_util.h"
#include "server_log.h"
#include <fcntl.h>
#include "server.h"

#include <openssl/ssl.h>

#define NAME_CMP(x) !strncmp(header->name, x, HTTP_HEAD_NAME_LEN)

static u8* hash_key(const char* key)
{

    size_t len = strlen(key);

    char* combined = calloc(1, len + sizeof(WEBSOCKET_GUID));
    strcat(combined, key);
    strcat(combined, WEBSOCKET_GUID);

    SHA_CTX sha1_ctx;
    unsigned char* hash = calloc(1, SHA_DIGEST_LENGTH);

    SHA1_Init(&sha1_ctx);
    SHA1_Update(&sha1_ctx, combined, strlen(combined));
    SHA1_Final(hash, &sha1_ctx);
    
    free(combined);

    return hash;
}

static char *b64_encode(const unsigned char *buffer, size_t len) 
{
    BIO* bio;
    BIO* b64;
    BUF_MEM* buffer_ptr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, buffer, len);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &buffer_ptr);
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);

    return buffer_ptr->data;
}

char* compute_websocket_key(char* key)
{
    unsigned char* hash = hash_key(key);
    char* b64 = b64_encode(hash, strlen((char*)hash));
    free(hash);
    return b64;
}


http_to_str_t http_to_str(const http_t* http)
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
    if (http->body)
        strcat(to_str.str, http->body);

    to_str.len = strnlen(to_str.str, to_str.max);

    //printf("HTTP to string (%zu):\n%s\n================End.\n", to_str.len, to_str.str);

    return to_str;
}

static void set_client_connection(client_t* client, http_header_t* header)
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

static void handle_http_upgrade(client_t* client, http_t* http, http_header_t* header)
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

static void handle_websocket_key(client_t* client, http_t* http, http_header_t* header)
{
    http->websocket_key = compute_websocket_key(header->val);
    http->websocket_key[28] = 0x00;
}

static void handle_http_header(client_t* client, http_t* http, http_header_t* header)
{
    if (NAME_CMP("Content-Length"))
    {
        http->body_len = atoi(header->val);
    }
    else if (NAME_CMP("Connection"))
    {
        set_client_connection(client, header);
    }
    else if (NAME_CMP("Upgrade"))
    {
        handle_http_upgrade(client, http, header);
    }
    else if (NAME_CMP("Sec-WebSocket-Key"))
    {
        handle_websocket_key(client, http, header);
    }
}

static http_t* parse_http(client_t* client, char* buf, size_t buf_len)
{

    http_t* http;
    char* saveptr;
    char* token;
    char* header;
    char* header_line;

    http = calloc(1, sizeof(http_t));

    header = strsplit(buf, HTTP_END, &saveptr);
    if (!header)
    {
        warn("parse_http first token is NULL!?!\n");
        free(http);
        return NULL;
    }
    http->body = strsplit(NULL, HTTP_END, &saveptr);
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
        http_header_t* header = &http->headers[http->n_headers];
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

        strncpy(header->name, name, HTTP_HEAD_NAME_LEN);
        strncpy(header->val, val, HTTP_HEAD_VAL_LEN);

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

    return http;
}

void print_parsed_http(const http_t* http)
{
    debug("Parsed HTTP: %s\n", (http->type == HTTP_REQUEST) ? "Request" : "Respond");
    if (http->type == HTTP_REQUEST)
        debug("\tMethod: '%s'\n\t\t\tURL: '%s'\n\t\t\tVersion: '%s'\n", http->req.method, http->req.url, http->req.version);
    else
        debug("\tVersion: '%s'\n\t\t\tCode: %u\n\t\t\tStatus: '%s'\n", http->resp.version, http->resp.code, http->resp.msg);

    for (size_t i = 0; i < http->n_headers; i++)
    {
        const http_header_t* header = &http->headers[i];

        debug("H:\t'%s' = '%s'\n", header->name, header->val);
    }

    if (http->body)
        debug("BODY (strlen: %zu, http: %zu):\t'%s'\n", strlen(http->body), http->body_len, (http->body) ? http->body : "NULL");
}

static void http_free(http_t* http)
{
    if (!http)
        return;

    if (http->body && http->body_inheap)
        free(http->body);

    free(http);
}

const char* http_get_header(http_t* http, const char* name)
{
    for (size_t i = 0; i < http->n_headers; i++)
    {
        http_header_t* header = http->headers + i;

        if (NAME_CMP(name))
            return header->val;
    }

    return NULL;
}

static void http_add_header(http_t* http, const char* name, const char* val)
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

static void server_upgrade_client_to_websocket(server_t* server, client_t* client, http_t* req_http)
{
    http_t* http = http_new_resp(HTTP_CODE_SW_PROTO, "Switching Protocols", NULL, 0);
    http_add_header(http, "Connection", HTTP_HEAD_CONN_UPGRADE);
    http_add_header(http, "Upgrade", "websocket");
    http_add_header(http, HTTP_HEAD_WS_ACCEPT, req_http->websocket_key);

    if (http_send(client, http) != -1)
        client->state |= CLIENT_STATE_WEBSOCKET;
}

static void server_handle_client_upgrade(server_t* server, client_t* client, http_t* http)
{
    const char* upgrade = http_get_header(http, HTTP_HEAD_CONN_UPGRADE);
    if (upgrade == NULL)
    {
        warn("Client fd:%d (Upgrade pending): No upgrade header in HTTP.\n", client->addr.sock);
        client->state ^= CLIENT_STATE_UPGRADE_PENDING;
        return;
    }

    if (!strncmp(upgrade, "websocket", HTTP_HEAD_VAL_LEN))
        server_upgrade_client_to_websocket(server, client, http);
    else
        warn("Connection upgrade '%s' not implemented.\n", upgrade);

    client->state ^= CLIENT_STATE_UPGRADE_PENDING;
}

static void http_add_body(http_t* restrict http, const char* restrict body, size_t body_len)
{
    http->body = calloc(1, body_len);
    strncpy(http->body, body, body_len);
    http->body_inheap = true;

    char val[HTTP_HEAD_VAL_LEN];
    snprintf(val, HTTP_HEAD_VAL_LEN, "%zu", body_len);

    http_add_header(http, HTTP_HEAD_CONTENT_LEN, val);
}

http_t* http_new_resp(u16 code, const char* status_msg, const char* body, size_t body_len)
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

static void server_http_resp_ok(client_t* client, char* content, size_t content_len, const char* content_type)
{
    http_t* http = http_new_resp(HTTP_CODE_OK, "OK", content, content_len);
    http_add_header(http, HTTP_HEAD_CONTENT_TYPE, content_type);

    http_send(client, http);

    http_free(http);
}

static void server_http_resp_error(client_t* client, u16 error_code, const char* status_msg)
{
    http_t* http = http_new_resp(error_code, status_msg, NULL, 0);

    http_send(client, http);

    http_free(http);
}

static void server_http_resp_404_not_found(client_t* client)
{
    const char* not_found_html = "<h1>Not Found</h1>";
    size_t len = strlen(not_found_html);

    http_t* http = http_new_resp(HTTP_CODE_NOT_FOUND, "Not Found", not_found_html, len);

    http_send(client, http);

    http_free(http);
}

static void server_http_do_get_req(server_t* server, client_t* client, http_t* http)
{
    size_t url_len = strnlen(http->req.url, HTTP_URL_LEN);
    char path[PATH_MAX];
    memset(path, 0, PATH_MAX);
    const char* content_type = "text/plain";
    i32 fd;
    size_t content_len;
    char* content;

    // TODO: Add "default_file_per_dir" config, default should be "index.html"
    if (http->req.url[url_len - 1] == '/')
        snprintf(path, PATH_MAX, "%s%s%s", server->conf.root_dir, http->req.url, "index.html"); 
    else
        snprintf(path, PATH_MAX, "%s%s", server->conf.root_dir, http->req.url); 
        //strncpy(path, http->req.url, HTTP_URL_LEN);
    
    if (file_isdir(path))
        strcat(path, "/index.html");

    if (strstr(path, ".html"))
        content_type = "text/html";
    else if (strstr(path, ".css"))
        content_type = "text/css";
    else if (strstr(path, ".js"))
        content_type = "application/javascript";
    else if (strstr(path, ".ico"))
        content_type = "image/x-icon";
    else
        warn("Path: '%s' dont know what content-type should be.\n");

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

    debug("Got file: '%s'\n", path);
    server_http_resp_ok(client, content, content_len, content_type);
}

static void server_handle_http_get(server_t* server, client_t* client, http_t* http)
{
    if (client->state & CLIENT_STATE_UPGRADE_PENDING)
        server_handle_client_upgrade(server, client, http);
    else
        server_http_do_get_req(server, client, http);
}

static void server_handle_http_req(server_t* server, client_t* client, http_t* http)
{
#define HTTP_CMP_METHOD(x) strncmp(http->req.method, x, HTTP_METHOD_LEN)

    if (!HTTP_CMP_METHOD("GET"))
        server_handle_http_get(server, client, http);
    else
        debug("Need to implement '%s' HTTP request.\n", http->req.method);
}

static void server_handle_http_resp(server_t* server, client_t* client, http_t* http)
{

}

void server_http_parse(server_t* server, client_t* client, u8* buf, size_t buf_len)
{
    // debug("Recv string from client:=============\n%s\n====================", buf);

    http_t* http = parse_http(client, (char*)buf, buf_len);
    if (!http)
    {
        error("parse_http() returned NULL, deleting client.\n");
        server_del_client(server, client);
        return;
    }

    print_parsed_http(http);

    if (http->type == HTTP_REQUEST)
        server_handle_http_req(server, client, http);
    else if (http->type == HTTP_RESPOND)
        server_handle_http_resp(server, client, http);
    else
        warn("Unknown http type: %d. Request or Respond? Ignored.\n", http->type);

    http_free(http);
}

ssize_t http_send(client_t* client, http_t* http)
{
    ssize_t bytes_sent = 0;
    http_to_str_t to_str = http_to_str(http);

    debug("HTTP send to fd:%d (%s:%s)\n%s\n", client->addr.sock, client->addr.ip_str, client->addr.serv, to_str.str);

    if ((bytes_sent = send(client->addr.sock, to_str.str, to_str.len, 0)) == -1)
    {
        error("HTTP send to (fd: %d, IP: %s:%s): %s\n",
            client->addr.sock, client->addr.ip_str, client->addr.serv
        );
    }

    free(to_str.str);

    return bytes_sent;
}