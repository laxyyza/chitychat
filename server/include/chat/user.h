#ifndef _SERVER_CHAT_USER_H_
#define _SERVER_CHAT_USER_H_

#include "common.h"
#include "chat/db_def.h"
#include "chat/rtusm.h"

typedef struct dbuser
{
    u32     user_id;
    char    username[DB_USERNAME_MAX];
    char    displayname[DB_DISPLAYNAME_MAX];
    char    bio[DB_BIO_MAX];
    u8      hash[SERVER_HASH_SIZE];
    u8      salt[SERVER_SALT_SIZE];
    char    created_at[DB_TIMESTAMP_MAX];
    char    pfp_hash[DB_PFP_NAME_MAX];
    i32     flags;
    rtusm_t rtusm;
} dbuser_t;

#endif // _SERVER_CHAT_USER_H_
