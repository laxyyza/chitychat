#ifndef _SERVER_HTTP_H_
#define _SERVER_HTTP_H_

#include "common.h"
#include "server_client.h"

#define WEBSOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define HTTP_END "\r\n\r\n"
#define HTTP_NL "\r\n"

#define HTTP_METHOD_LEN 16
#define HTTP_VERSION_LEN 16
#define HTTP_URL_LEN 256
#define HTTP_HEAD_NAME_LEN 64
#define HTTP_HEAD_VAL_LEN 256
#define HTTP_MAX_HEADERS 20
#define HTTP_CODE_LEN 6
#define HTTP_STATUS_MSG_LEN 128

#define HTTP_CODE_SW_PROTO 101
#define HTTP_CODE_OK 200
#define HTTP_CODE_BAD_REQ 400
#define HTTP_CODE_NOT_FOUND 404
#define HTTP_CODE_INTERAL_ERROR 500

#define HTTP_HEAD_CONTENT_LEN "Content-Length"
#define HTTP_HEAD_WS_ACCEPT   "Sec-WebSocket-Accept"
#define HTTP_HEAD_CONN_UPGRADE "Upgrade"

enum http_keep_alive
{
    HTTP_CONN_UNKNOWN = 0,
    HTTP_CONN_KEEP_ALIVE,
    HTTP_CONN_CLOSE,
};

enum http_type
{
    HTTP_REQUEST,
    HTTP_RESPOND
};

typedef struct 
{
    char method[HTTP_METHOD_LEN];
    char url[HTTP_URL_LEN];
    char version[HTTP_VERSION_LEN];
} http_req_t;

typedef struct 
{
    char version[HTTP_VERSION_LEN];
    uint8_t code;
    char msg[HTTP_STATUS_MSG_LEN];
} http_resp_t;
 
typedef struct 
{
    char name[HTTP_HEAD_NAME_LEN];
    char val[HTTP_HEAD_VAL_LEN];
} http_header_t;

typedef struct
{
    enum http_type type;
    union {
        http_req_t req;
        http_resp_t resp;
    };
    http_header_t headers[HTTP_MAX_HEADERS];
    size_t n_headers;
    char* body;
    size_t body_len;

    struct {
        enum http_keep_alive keep_alive;
        char* websocket_key;
        bool body_inheap;
    };
} http_t;

typedef struct 
{
    char* str;
    size_t len;
    size_t max;
} http_to_str_t;

void server_http_parse(server_t* server, client_t* client, u8* buf, size_t buf_len);

#endif // _SERVER_HTTP_H_