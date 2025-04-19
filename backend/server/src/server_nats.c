#include "server.h"
#include "server_nats.h"
#include "backend.h"

static enum se_status
nats_read(UNUSED eworker_t* ew, server_event_t* ev)
{
	natsConnection_ProcessReadEvent(ev->data);
	return SE_OK;
}

static enum se_status
nats_write(UNUSED eworker_t* ew, server_event_t* ev)
{
	natsConnection_ProcessWriteEvent(ev->data);
	return SE_OK;
}

static enum se_status
nats_close(UNUSED eworker_t* ew, UNUSED server_event_t* ev)
{
	warn("nats_close. TODO: Implement NATS Close!\n");
	return SE_OK;
}

natsStatus
nats_attach(void** user_data, void* loop, natsConnection* nc, natsSock sock)
{
	server_t* server = loop;

	server->nats.ev = server_epoll_add_event(server, sock, nc, 
									nats_read, 
									nats_write, 
									nats_close,
                                    "NATS");
	server->nats.conn = nc;
	server->nats.server = server;
	*user_data = &server->nats;

	return NATS_OK;
}

natsStatus
nats_set_poll_read(void* user_data, bool add)
{
	server_nats_t* nats = user_data;
	if (add)
		nats->ev->new_listen_events |= EPOLLIN;
	else
		nats->ev->new_listen_events &= ~EPOLLIN;
	return NATS_OK;
}

natsStatus
nats_set_poll_write(void* user_data, bool add)
{
	server_nats_t* nats = user_data;
	if (add)
		nats->ev->new_listen_events |= EPOLLOUT;
	else
		nats->ev->new_listen_events &= ~EPOLLOUT;

	if (nats->ev->new_listen_events != nats->ev->listen_events)
		server_epoll_rearm(nats->server, nats->ev);

	return NATS_OK;
}

natsStatus
nats_detach(UNUSED void* user_data)
{
	warn("nats detach: Implement NATS Detach!\n", user_data);
	return NATS_OK;
}

static void 
cc_server_http_msg(UNUSED natsConnection* nc, UNUSED natsSubscription* sub, natsMsg* msg, server_t* server)
{
	const char* data = natsMsg_GetData(msg);
	const u32 size = natsMsg_GetDataLength(msg);

	json_tokener* tokener = json_tokener_new();
	json_object* json = json_tokener_parse_ex(tokener, data, size);
	json_tokener_free(tokener);

	if (json == NULL)
		goto msg_free;

	backend_read(server, json);
	
	json_object_put(json);

msg_free:
	natsMsg_Destroy(msg);
}

bool
server_init_nats(server_t* server)
{
	natsStatus s;

	s = natsOptions_Create(&server->nats.opts);
	if (s != NATS_OK)
	{
		fatal("Failed to create NATS Options. Out of memory?\n");
		return false;
	}

	s = natsOptions_SetEventLoop(server->nats.opts, server, 
						nats_attach, 
						nats_set_poll_read, 
						nats_set_poll_write, 
						nats_detach);
	if (s != NATS_OK)
	{
		fatal("Failed to set NATS event loop!\n");
		return false;
	}

	s = natsConnection_Connect(&server->nats.conn, server->nats.opts);
	if (s != NATS_OK)
	{
		fatal("Failed to connect to NATS server!\n");
		return false;
	}

	natsConnection_GetClientID(server->nats.conn, &server->nats.client_id);

	verbose("NATS CLIENT ID: %lu\n", server->nats.client_id);

	snprintf(server->nats.subj_http, SUBJECT_LEN - 1, "cc_server.http.%lu", server->nats.client_id);
	snprintf(server->nats.subj_ws, SUBJECT_LEN - 1, "cc_server.ws.%lu", server->nats.client_id);

	natsConnection_PublishString(server->nats.conn, "new_cc_server", "Yup a new one!");

	natsConnection_Subscribe(&server->nats.sub_http, server->nats.conn, server->nats.subj_http, (void*)cc_server_http_msg, server);

	natsSubscription_SetPendingLimits(server->nats.sub_http, -1, -1);

	return true;
}

