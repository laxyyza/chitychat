#include "backend_route.h"
#include "server_client.h"
#include "server.h"
#include "backend.h"

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
            return;
        }
	}
}

bool 
backend_route(server_t* server, client_t* client, http_t* http)
{
	const array_t* routes = &server->backend_routes.routes;

	for (u32 i = 0; i < routes->count; i++)
	{
		const backend_route_t* route = (backend_route_t*)array_idx(routes, i);

		if (strncmp(route->path_prefix, http->req.url, route->path_len) == 0)
		{
			backend_send_http(server, route->service, client, http);
			return true;
		}
	}
	return false;
}
