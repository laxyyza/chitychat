#ifndef _SERVER_DB_LOGIN_ATTEMPTS_H_
#define _SERVER_DB_LOGIN_ATTEMPTS_H_

#include "chat/db_def.h"

bool db_async_login_attempt(server_db_t* db, 
                            const char* username, 
                            bool successful, 
                            const char* user_agent, 
                            const char* ip_address);

#endif // _SERVER_DB_LOGIN_ATTEMPTS_H_
