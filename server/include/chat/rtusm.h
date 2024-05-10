#ifndef _SERVER_CHAT_RTUSM_H_
#define _SERVER_CHAT_RTUSM_H_

/*
 * RTUSM - Real-Time User Status Management
 *
 * e.g. Online, Offline, Away, typing, etc.
 * and oewer user updates like profile pic changes
 */

#include "server_tm.h"

typedef struct dbuser dbuser_t;

enum rtusm_status 
{
    USER_OFFLINE,
    USER_ONLINE,
    USER_AWAY,
    USER_DND,

    RTUSM_STATUS_LEN
};

typedef struct 
{
    enum rtusm_status status;
    u8                force_status:1; // TODO: Save user-set status into database.
    u8                typing:1;
    u32               typing_group_id;
} rtusm_t;

void    server_rtusm_set_user_status(eworker_t* ew, dbuser_t* user, 
                                     enum rtusm_status status);
void    server_rtusm_user_connect(eworker_t* ew, dbuser_t* user);
void    server_rtusm_user_disconnect(eworker_t* ew, dbuser_t* user);
void    server_rtusm_user_pfp_change(eworker_t* ew, dbuser_t* user);

const char* rtusm_get_status_str(enum rtusm_status status);

#endif // _SERVER_CHAT_RTUSM_H_
