#ifndef _SERVER_DB_REMEMBER_TOKEN_USAGE_H_
#define _SERVER_DB_REMEMBER_TOKEN_USAGE_H_

#include "chat/db_def.h"

bool db_async_remember_token_usage(server_db_t* db, 
                                    u32 token_id,
                                    const char* user_agent, 
                                    const char* ip_address);

#endif // _SERVER_DB_REMEMBER_TOKEN_USAGE_H_
