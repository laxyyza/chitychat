#ifndef _SERVER_USER_GROUP_H_
#define _SERVER_USER_GROUP_H_

#include "common.h"
#include "server_websocket.h"

const char* server_group_create(server_thread_t* th, 
                                client_t* client, 
                                json_object* payload, 
                                json_object* respond_json);

const char* server_client_groups(server_thread_t* th, 
                                 client_t* client, 
                                 json_object* respond_json);

const char* server_get_all_groups(server_thread_t* th, 
                                  client_t* client, 
                                  json_object* respond_json);

const char* server_join_group(server_thread_t* th, 
                              client_t* client, 
                              json_object* payload, 
                              json_object* respond_json);

const char* server_get_send_group_msg(server_thread_t* th,
                                      const dbmsg_t* msg, 
                                      const u32 group_id);

const char* server_group_msg(server_thread_t* th, 
                             client_t* client, 
                             json_object* payload, 
                             json_object* respond_json);

const char* get_group_msgs(server_thread_t* th, 
                           client_t* client, 
                           json_object* payload, 
                           json_object* respond_json);

const char* server_create_group_code(server_thread_t* th, 
                                     client_t* client,
                                     json_object* payload, 
                                     json_object* respond_json);

const char* server_join_group_code(server_thread_t* th,
                                   client_t* client,
                                   json_object* payload,
                                   json_object* respond_json);

#endif // _SERVER_USER_GROUP_H_
