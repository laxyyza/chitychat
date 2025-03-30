#ifndef _SERVER_CHAT_USER_H_
#define _SERVER_CHAT_USER_H_

#include "common.h"
#include "chat/db_def.h"
#include "chat/rtusm.h"
#include "array.h"
#include <nats.h>

typedef struct client client_t;

#define TOKEN_CAP 10
#define TOKEN_REFIL_RATE 1
#define RATE_LIMIT_RESPOND_RATE 20
#define RATE_LIMIT_DISCONNECT -30

typedef struct 
{
    i32 tokens;
    f64 last_token_refil;
    f64 last_respond_time;
} token_bucket_t;

typedef struct dbuser
{
    u32     user_id;
    char    username[DB_USERNAME_MAX];
    char    displayname[DB_DISPLAYNAME_MAX];
    char    bio[DB_BIO_MAX];
    u8      hash[SERVER_HASH_SIZE];
    u8      salt[SERVER_SALT_SIZE];
    char    created_at[DB_TIMESTAMP_MAX];
    char    pfp_hash[DB_PFP_HASH_MAX];
    i32     flags;
    rtusm_t rtusm;

    struct {
        array_t connected_clients;
        token_bucket_t msg_tokens;
        natsSubscription* sub;
    };
} dbuser_t;

dbuser_t* server_new_user(eworker_t* ew, u32 user_id);
bool      server_userid_send(eworker_t* ew, u32 user_id, json_object* packet);
void      server_user_send(dbuser_t* user, json_object* packet);

const char* server_client_user_info(eworker_t* ew, 
                                    client_t* client, 
                                    json_object* payload,
                                    json_object* respond_json);

const char* server_get_user(eworker_t* ew, 
                            client_t* client, 
                            json_object* payload, 
                            json_object* respond_json);

const char* server_user_edit_account(eworker_t* ew, 
                                     client_t* client, 
                                     json_object* payload, 
                                     json_object* respond_json);

#endif // _SERVER_CHAT_USER_H_
