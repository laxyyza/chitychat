#ifndef _BACKEND_ROUTE_H_
#define _BACKEND_ROUTE_H_

#include "common.h"
#include "server_client.h"

#define ROUTE_PREFIX_MAX 127

bool backend_route(eworker_t* ew, client_t* client, http_t* http);

#endif // _BACKEND_ROUTE_H_
