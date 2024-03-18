#ifndef _SERVER_GROUP_H_
#define _SERVER_GROUP_H_

#include "common.h"
#include "server_websocket.h"

const char* server_group_create(server_t* server, client_t* client, 
        json_object* payload, json_object* respond_json);
const char* server_client_groups(server_t* server, client_t* client, 
        json_object* respond_json);
const char* server_get_all_groups(server_t* server, client_t* client, 
        json_object* respond_json);
const char* server_join_group(server_t* server, client_t* client, 
        json_object* payload, json_object* respond_json);
const char* server_group_msg(server_t* server, client_t* client, 
        json_object* payload, json_object* respond_json);
const char* get_group_msgs(server_t* server, client_t* client, 
        json_object* payload, json_object* respond_json);

#endif // _SERVER_GROUP_H_