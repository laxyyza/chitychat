#ifndef _SERVER_NATS_H_
#define _SERVER_NATS_H_

#include "common.h"
#include <nats.h>
#include "server_events.h"

#define SUBJECT_LEN 64

typedef struct 
{
	natsConnection* conn;
	natsOptions* opts;
	server_t* server;
	server_event_t* ev;
	u64 client_id;
	char subj_http[SUBJECT_LEN];
} server_nats_t;

bool server_init_nats(server_t* server);

#endif // _SERVER_NATS_H_