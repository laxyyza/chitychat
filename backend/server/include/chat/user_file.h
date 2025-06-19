#ifndef _SERVER_FILE_H_
#define _SERVER_FILE_H_

#include "common.h"

bool            server_init_magic(server_t* server);
void            server_close_magic(server_t* server);
const char*     server_mime_type(server_t* server, const void* data, 
                                                        size_t size);

#endif // _SERVER_FILE_H_
