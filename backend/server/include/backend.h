#ifndef _BACKEND_H_
#define _BACKEND_H_

#include "common.h"
#include "backend_service.h"
#include "server_client.h"

typedef struct server server_t;

bool backend_socket_init(server_t* server);
void backend_services_close(server_t* server);
void backend_send_http(server_t* server, backend_service_t* bs, client_t* client, http_t* http);

#endif // _BACKEND_H_
