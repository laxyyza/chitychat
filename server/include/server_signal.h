#ifndef _SERVER_SIGNAL_H_
#define _SERVER_SIGNAL_H_

#include "common.h"
#include <sys/signalfd.h>

typedef struct 
{
    i32      fd;
    sigset_t mask;
} server_signal_t;

bool server_init_signal(server_t* server);
void server_wait_for_signals(server_t* server);

#endif // _SERVER_SIGNAL_H_
