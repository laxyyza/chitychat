#ifndef _SERVER_WS_PLD_HDLR_H_
#define _SERVER_WS_PLD_HDLR_H_

#include "server.h"

#define JSON_INVALID_STR(key) "\""key"\"" " is invalid or not found"
#define RET_IF_JSON_BAD(dest_json, src_json, key, type)\
        dest_json = json_object_object_get(src_json, key);\
        if (json_bad(dest_json, type))\
            return JSON_INVALID_STR(key)

bool json_bad(json_object* json, json_type type);

enum client_recv_status server_ws_handle_text_frame(eworker_t* th, client_t* client, 
                                                    char* buf, size_t buf_len);

#endif // _SERVER_WS_PLD_HDLR_H_
