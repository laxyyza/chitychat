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
	debug("nats_close. TODO: Implement NATS Close!\n");
	return SE_OK;
}

natsStatus
nats_attach(void** user_data, void* loop, natsConnection* nc, natsSock sock)
{
	eworker_t* ew = loop;

    add_event_args_t args = {
        .fd = sock,
        .data = nc,
        .read_cb = nats_read,
        .write_cb = nats_write, 
        .close_cb = nats_close,
        .name = "NATS",
        .type = FD_EXCLUSIVE
    };
    ew->server->nats.ev = server_epoll_add_event(ew, &args);
	// ew->server->nats.ev = server_epoll_add_event(server, sock, nc, 
	// 								nats_read, 
	// 								nats_write, 
	// 								nats_close,
	//                                    "NATS");
	ew->server->nats.conn = nc;
	ew->server->nats.server = ew->server;
	*user_data = &ew->server->nats;

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
		eworker_epoll_rearm(nats->server->main_ew, nats->ev);

	return NATS_OK;
}

natsStatus
nats_detach(void* user_data)
{
    server_nats_t* nats = user_data;
    eworker_del_event(nats->server->main_ew, nats->ev);
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
server_init_nats(eworker_t* ew)
{
	natsStatus s;
    server_t* server = ew->server;
    const char* url;

    /*
     * Because the NATS.C library is a fucking stupid library that insists on creating threads
     * (seriously, what the fuck — you don’t need threads for a networking library!
     * Ever heard of I/O multiplexing?!),
     * I have to block SIGINT and SIGTERM on *all* threads (including NATS threads),
     * so I can catch signals myself and perform a proper graceful shutdown.
     */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);

	s = natsOptions_Create(&server->nats.opts);
	if (s != NATS_OK)
	{
		fatal("Failed to create NATS Options. Out of memory?\n");
		return false;
	}

    url = getenvd("NATS_URL", NATS_DEFAULT_URL);

    natsOptions_SetURL(server->nats.opts, url);

	s = natsOptions_SetEventLoop(ew->server->nats.opts, ew, 
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
    natsOptions_Destroy(server->nats.opts);

	natsConnection_GetClientID(server->nats.conn, &server->nats.client_id);

	verbose("NATS CLIENT ID: %lu\n", server->nats.client_id);

	snprintf(server->nats.subj_http, SUBJECT_LEN - 1, "cc_server.http.%lu", server->nats.client_id);
	snprintf(server->nats.subj_ws, SUBJECT_LEN - 1, "cc_server.ws.%lu", server->nats.client_id);

	natsConnection_Subscribe(&server->nats.sub_http, server->nats.conn, server->nats.subj_http, (void*)cc_server_http_msg, server);

	natsSubscription_SetPendingLimits(server->nats.sub_http, -1, -1);

	return true;
}

void 
server_deinit_nats(server_t* server)
{
    natsSubscription_Destroy(server->nats.sub_http);
    natsConnection_Destroy(server->nats.conn);
    nats_Close();
}

void 
server_nats_user_event(UNUSED natsConnection* nc, UNUSED natsSubscription* sub, natsMsg* msg, void* closuer)
{
    const char* data = natsMsg_GetData(msg);
    const u32 size = natsMsg_GetDataLength(msg);
    dbuser_t* user = closuer;

    json_tokener* tok = json_tokener_new();
    json_object* json = json_tokener_parse_ex(tok, data, size);
    json_tokener_free(tok);

    if (json == NULL)
        goto free_msg;

    server_user_send(user, json);

    json_object_put(json);

free_msg:
    natsMsg_Destroy(msg);
}

