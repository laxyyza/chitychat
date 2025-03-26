#include "backend_route.h"
#include "server_client.h"
#include "server.h"
#include "backend.h"
#include "chat/db_user_session.h"

void 
backend_route_init(backend_route_table_t* brt)
{
	array_init(&brt->routes, sizeof(backend_route_t), 10);
}

void 
backend_route_deinit(backend_route_table_t* brt)
{
	array_del(&brt->routes);
}

void 
backend_route_add(backend_route_table_t* brt, const char* path_prefix, backend_service_t* bs)
{
	backend_route_t* new_route = array_add_into(&brt->routes);
	strncpy(new_route->path_prefix, path_prefix, ROUTE_PREFIX_MAX);
	new_route->service = bs;
	new_route->path_len = strnlen(new_route->path_prefix, ROUTE_PREFIX_MAX);

	info("BS '%s' registered: %s\n", bs->name, path_prefix);
}

void 
backend_route_del(backend_route_table_t* brt, const backend_service_t* bs)
{
	array_t* routes = &brt->routes;

	for (u32 i = 0; i < routes->count; i++)
	{
		const backend_route_t* route = (backend_route_t*)array_idx(routes, i);

        if (route->service == bs)
        {
            info("Unregister: %s from bs:%s\n", route->path_prefix, bs->name);
            array_erase(routes, i);
            i--;
        }
	}
}

static const char* 
do_get_session(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    dbsession_t* session = ctx->data;
    const backend_route_t* route = ctx->param.session_route.route;
    http_t* http = ctx->param.session_route.http;

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        server_http_resp_error(ctx->client, HTTP_CODE_UNAUTHORIZED, HTTP_UNAUTHORIZED);
        http_free(http);
        return "Invalid session ID";
    }

    backend_send_http(ew->server, route->service, ctx->client, http, session->user_id);

    http_free(http);

    return NULL;
}

static void 
backend_do_route(eworker_t* ew, const backend_route_t* route, client_t* client, http_t* http)
{
    if (http->session_uuid == NULL)
    {
        server_http_resp_error(client, HTTP_CODE_UNAUTHORIZED, HTTP_UNAUTHORIZED);
        return;
    }

    client_t* real_client = server_ght_get(&ew->server->client_by_session_ht, 
                                           server_ght_hash_uuid(http->session_uuid));

    if (real_client && real_client->dbuser)
    {
        backend_send_http(ew->server, route->service, client, http, real_client->dbuser->user_id);
    }
    else
    {
        dbsession_t* session = calloc(1, sizeof(dbsession_t));
        strncpy(session->uuid, http->session_uuid, UUID_LEN - 1);

        dbcmd_ctx_t ctx = {
            .exec = do_get_session,
            .param.session_route.http = http,
            .param.session_route.route = route
        };
        ew->ignore_http_free = true;

        db_async_select_session(&ew->db, session, &ctx);
    }
}

bool 
backend_route(eworker_t* ew, client_t* client, http_t* http)
{
	const array_t* routes = &ew->server->backend_routes.routes;

    for (u32 i = 0; i < routes->count; i++)
	{
		const backend_route_t* route = (backend_route_t*)array_idx(routes, i);

		if (strncmp(route->path_prefix, http->req.url, route->path_len) == 0)
		{
            backend_do_route(ew, route, client, http);
			return true;
		}
	}
	return false;
}
