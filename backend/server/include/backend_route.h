#ifndef _BACKEND_ROUTE_H_
#define _BACKEND_ROUTE_H_

#include "common.h"
#include "array.h"
#include "backend_service.h"
#include "server_client.h"

#define ROUTE_PREFIX_MAX 127

typedef struct 
{
    char path_prefix[ROUTE_PREFIX_MAX + 1];
    u32  path_len;
    backend_service_t* service;
} backend_route_t;

typedef struct 
{
    array_t routes;
} backend_route_table_t;

void backend_route_init(backend_route_table_t* brt);
void backend_route_deinit(backend_route_table_t* brt);
void backend_route_add(backend_route_table_t* brt, const char* path_prefix, backend_service_t* bs);
void backend_route_del(backend_route_table_t* brt, const backend_service_t* bs);
bool backend_route(eworker_t* ew, client_t* client, http_t* http);

#endif // _BACKEND_ROUTE_H_
