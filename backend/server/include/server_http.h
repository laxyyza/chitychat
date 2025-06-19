#ifndef _SERVER_HTTP_H_
#define _SERVER_HTTP_H_

#include "common.h"
#include "server_client.h"
#include "server_tm.h"

#define WEBSOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define HTTP_END "\r\n\r\n"
#define HTTP_NL "\r\n"
// #define HTTP_VERSION "HTTP/3"
#define HTTP_VERSION "HTTP/1.1"

#define HTTP_METHOD_LEN     16
#define HTTP_VERSION_LEN    16
#define HTTP_URL_LEN        256
#define HTTP_HEAD_NAME_LEN  64
#define HTTP_HEAD_VAL_LEN   256
#define HTTP_CODE_LEN       6
#define HTTP_STATUS_MSG_LEN 128

#define HTTP_MAX_HEADERS    30
#define HTTP_MAX_PARAMS     10

/* 1XX information response */
#define HTTP_CODE_CONTINUE          100
#define HTTP_CODE_SW_PROTO          101
#define HTTP_CODE_EARLY_HINTS       103

/* 2XX success */
#define HTTP_CODE_OK                200
#define HTTP_CODE_CREATED           201
#define HTTP_CODE_ACCEPTED          202
#define HTTP_CODE_NO_CONTENT        204
#define HTTP_CODE_RESET_CONTENT     205
#define HTTP_CODE_PARTIAL_CONTENT   206
#define HTTP_CODE_IM_USED           226

/* 3XX redirection */
// Not implemented.

/* 4XX client errors */
#define HTTP_CODE_BAD_REQ           400
#define HTTP_CODE_UNAUTHORIZED      401
#define HTTP_CODE_FORBIDDEN         403
#define HTTP_CODE_NOT_FOUND         404
#define HTTP_CODE_METH_NOT_ALLOW    405
#define HTTP_CODE_NOT_ACCEPTABLE    406
#define HTTP_CODE_REQ_TIMEOUT       408
#define HTTP_CODE_CONFLICT          409
#define HTTP_CODE_GONE              410
#define HTTP_CODE_LEN_REQUIRED      411
#define HTTP_CODE_PRECON_FAILED     412
#define HTTP_CODE_PAYLOAD_LARGE     413
#define HTTP_CODE_URI_TOO_LONG      414
#define HTTP_CODE_UNSUPP_MEDIA      415
#define HTTP_CODE_RANGE_NOT_SAT     416
#define HTTP_CODE_EXPECTATION_F     417
#define HTTP_CODE_MISDIRECTED_R     421
#define HTTP_CODE_UNPROCESS_CONTENT 422
#define HTTP_CODE_TOO_EARLY         425
#define HTTP_CODE_UPGRADE_REQUIRED  426
#define HTTP_CODE_PRECOND_REQUIRED  428
#define HTTP_CODE_TOO_MANY_REQUESTS 429
#define HTTP_CODE_HEADERS_TO_LARGE  431

/* 5XX server errors */
#define HTTP_CODE_INTERAL_ERROR     500
#define HTTP_CODE_NOT_IMPLEMENT     501
#define HTTP_CODE_BAD_GATEWAY       502
#define HTTP_CODE_SERV_UNAVAIL      503
#define HTTP_CODE_GATEWAY_TIMEOUT   504
#define HTTP_CODE_VERSION_NOT_SUPP  505
#define HTTP_CODE_VARIANT_ALSO_NEGO 506
#define HTTP_CODE_NOT_EXTENDED      510
#define HTTP_CODE_NET_AUTH_REQUIRED 511

#define HTTP_HEAD_CONTENT_LEN "content-length"
#define HTTP_HEAD_WS_ACCEPT   "sec-webSocket-accept"
#define HTTP_HEAD_CONN_UPGRADE "upgrade"
#define HTTP_HEAD_CONTENT_TYPE "content-type"

#define HTTP_CMP_METHOD(x) strncmp(http->req.method, x, HTTP_METHOD_LEN)

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
    u16 code;
    char msg[HTTP_STATUS_MSG_LEN];
} http_resp_t;

typedef struct 
{
    char name[HTTP_HEAD_NAME_LEN];
    char val[HTTP_HEAD_VAL_LEN];
} http_header_t;

typedef struct http
{
    enum http_type type;
    union {
        http_req_t req;
        http_resp_t resp;
    };
    http_header_t headers[HTTP_MAX_HEADERS];
    size_t n_headers;
    http_header_t params[HTTP_MAX_PARAMS];
    size_t n_params;
    char* body;
    size_t body_len;
    size_t header_len;

    struct {
        enum http_keep_alive keep_alive;
        char* websocket_key;
        const char* origin;
        bool body_inheap;
    };

    struct {
        char* session_uuid;
        char* remember_token;
    } cookies;

    struct {
        bool missing;
        size_t total_recv;
    } buf;
} http_t;

typedef struct 
{
    char* str;
    size_t len;
    size_t max;
} http_to_str_t;

void                    server_http_switch_to_websocket(client_t* client);
enum client_recv_status server_http_parse(eworker_t* ew, client_t* client, u8* buf, 
                                          size_t buf_len);
enum client_recv_status server_handle_http(eworker_t* ew, client_t* client, http_t* http);
http_header_t*          http_get_header(const http_t* http, const char* name);
char*                   http_add_header_adv(http_t* http, const char* name, const char* val, bool override);
char*                   http_add_header(http_t* http, const char* name, const char* val);
http_t*                 http_new_resp(u16 code, const char* body, size_t body_len);
ssize_t                 http_send(client_t* client, http_t* http);
void                    http_free(http_t* http);
int                     server_http_url_checks(http_t* http);
void                    server_http_resp(client_t* client, u16 error_code);
void                    server_http_resp_json_single(client_t* client, u16 code, const char* key, const char* val);
void                    server_http_resp_error(client_t* client, u16 error_code, const char* error_msg);
void                    server_http_resp_404_not_found(client_t* client);
void                    http_add_cross_origin_headers(client_t* client, http_t* http);
void                    server_http_resp_ok(client_t* client, char* content, 
                                            size_t content_len, const char* content_type);

enum client_recv_status server_handle_http_get(server_t* server, client_t* client, http_t* http);

enum client_recv_status server_handle_http_post(eworker_t* ew, client_t* client, 
                                                const http_t* http);

const char* http_status_code_str(u32 code);

#endif // _SERVER_HTTP_H_
