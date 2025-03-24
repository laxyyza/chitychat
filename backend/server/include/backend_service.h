#ifndef _BACKEND_SERVICE_H_
#define _BACKEND_SERVICE_H_

#include "common.h"
#include "server_net.h"

#define SERIVCE_NAME_MAX 31

typedef struct 
{
    char name[SERIVCE_NAME_MAX + 1];
    net_addr_t addr;
    bool registered;
} backend_service_t;

#endif // _BACKEND_SERVICE_H_
