#ifndef _SERVER_FILE_H_
#define _SERVER_FILE_H_

#include "common.h"
#include "server_tm.h"
#include "chat/db.h"

bool            server_init_magic(server_t* server);
void            server_close_magic(server_t* server);
const char*     server_mime_type(server_t* server, const void* data, 
                                                        size_t size);
bool            server_save_file(server_thread_t* th, const void* data, 
                        size_t size, const char* name);
bool            server_save_file_img(server_thread_t* th, const void* data, 
                size_t size, const char* name, dbuser_file_t* file_output);
void*           server_get_file(server_thread_t* th, dbuser_file_t* file);
bool            server_delete_file(server_thread_t* th, dbuser_file_t* file);

#endif // _SERVER_FILE_H_
