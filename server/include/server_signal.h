#ifndef _SERVER_SIGNAL_H_
#define _SERVER_SIGNAL_H_

#include "common.h"
#include "server_tm.h"
#include "server_events.h"
#include <sys/signalfd.h>

typedef struct 
{
    i32 fd;
    sigset_t mask;
} server_signal_t;

bool    server_init_signal(server_t* server);
enum se_status server_close_signal(eworker_t* ew, server_event_t* ev);

#endif // _SERVER_SIGNAL_H_
