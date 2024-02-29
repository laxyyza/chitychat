#ifndef _SERVER_HTTP_H_
#define _SERVER_HTTP_H_

#include "common.h"
#include "server_client.h"

typedef struct 
{

} http_t;

void server_http_parse(server_t* server, client_t* client, u8* buf, size_t buf_len);

#endif // _SERVER_HTTP_H_