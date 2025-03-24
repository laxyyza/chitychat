#ifndef _BACKEND_H_
#define _BACKEND_H_

#include "common.h"

typedef struct server server_t;

bool backend_socket_init(server_t* server);
void backend_services_close(server_t* server);

#endif // _BACKEND_H_
