#ifndef _BACKEND_ROUTE_H_
#define _BACKEND_ROUTE_H_

#include "common.h"
#include "array.h"
#include "backend_service.h"

#define ROUTE_PREFIX_MAX 128

typedef struct 
{
    char path_prefix[ROUTE_PREFIX_MAX];
    backend_service_t* service;
} backend_route_t;

typedef struct 
{
    array_t routes;
} backend_route_table_t;

#endif // _BACKEND_ROUTE_H_
