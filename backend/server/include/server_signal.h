#ifndef _SERVER_SIGNAL_H_
#define _SERVER_SIGNAL_H_

#include "common.h"
#include "server_eworker.h"
#include <sys/signalfd.h>

typedef struct 
{
    i32      fd;
    sigset_t mask;
} server_signal_t;

void server_shutdown_and_notify(server_t* server, i32 return_code);
bool server_init_signal(eworker_t* ew);

#endif // _SERVER_SIGNAL_H_
