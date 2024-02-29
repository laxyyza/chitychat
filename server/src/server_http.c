#include "server_http.h"
#include "server_util.h"
#include "server_log.h"

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
            client->state |= CLIENT_STATE_UPGRADE_PEDDING;
        else
            warn("HTTP Connection: '%s' not implemented.\n", token);

        token = strtok_r(NULL, ", ", &saveptr);
    }
}

static void handle_http_upgrade(client_t* client, http_t* http, http_header_t* header)
{
    if (client->state & CLIENT_STATE_UPGRADE_PEDDING)
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
        printf("Body len: %zu\n", http->body_len);
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
    memset(http, 0, sizeof(http_t));

    header = strsplit(buf, HTTP_END, &saveptr);
    if (!header)
    {
        printf("parse_http first token is NULL!?!\n");
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
    printf("Parsed HTTP: %s\n", (http->type == HTTP_REQUEST) ? "Request" : "Respond");
    if (http->type == HTTP_REQUEST)
        printf("\tMethod: '%s'\n\tURL: '%s'\n\tVersion: '%s'\n", http->req.method, http->req.url, http->req.version);
    else
        printf("\tVersion: '%s'\n\tCode: %u\n\tStatus: '%s'\n", http->resp.version, http->resp.code, http->resp.msg);

    for (size_t i = 0; i < http->n_headers; i++)
    {
        const http_header_t* header = &http->headers[i];

        printf("H:\t'%s' = '%s'\n", header->name, header->val);
    }

    if (http->body)
        printf("BODY (strlen: %zu, http: %zu):\t'%s'\n", strlen(http->body), http->body_len, (http->body) ? http->body : "NULL");
}

static void http_free(http_t* http)
{
    if (!http)
        return;

    if (http->body && http->body_inheap)
        free(http->body);

    free(http);
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

    http_free(http);
}
