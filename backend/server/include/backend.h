#ifndef _BACKEND_H_
#define _BACKEND_H_

#include "common.h"
#include "backend_service.h"
#include "server_client.h"

typedef struct server server_t;

void backend_read(server_t* server, json_object* json);
void backend_send_http(server_t* server, const char* subject, client_t* client, http_t* http, u32 user_id);

#endif // _BACKEND_H_
