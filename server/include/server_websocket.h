#ifndef _SERVER_WEBSOCKET_H_
#define _SERVER_WEBSOCKET_H_

#include "common.h"
#include "server_tm.h"
#include "server_client.h"
                                    //      Opcode
                                    //      |ONLY|
#define WS_CONTINUE_FRAME   0x00    // 0b000|0000|
#define WS_TEXT_FRAME       0x01    // 0b000|0001|
#define WS_BINARY_FRAME     0x02    // 0b000|0010|
#define WS_CLOSE_FRAME      0x08    // 0b000|1000|
#define WS_PING_FRAME       0x09    // 0b000|1001|
#define WS_PONG_FRAME       0x0A    // 0b000|1010|

#define WS_FIN_BIT          0x80
#define WS_MASK_BIT         0x80
#define WS_OPCODE_BITS      0x0F
#define WS_PAYLOAD_LEN_BITS 0x7F

#define WS_FIN(frame)           frame[0] & WS_FIN_BIT >> 7      // 0b10000000 >> 7 0b00000001
#define WS_MASK(frame)          frame[1] & WS_MASK_BIT          // 0b10000000
#define WS_OPCODE(frame)        frame[0] & WS_OPCODE_BITS       // 0b00001111
#define WS_PAYLOAD_LEN(frame)   frame[1] & WS_PAYLOAD_LEN_BITS  // 0b01111111
#define WS_PAYLOAD_LEN16(frame) (frame[2] << 8) | frame[3];
#define WS_PAYLOAD_LEN64(frame) (uint64_t)frame[2];

/*
 * ws_frame_t - Web Socket Frame
 *      
 *      |     byte 0    |     byte 1    |    byte 2     |     byte 3    | Total: 2-14 bytes
 *      |0              |    1          |        2      |            3  |
 * bits: 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *      +-+-+-+-+-------+-+-------------+-------------------------------+
 *      |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 *      |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 *      |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 *      | |1|2|3|       |K|             |                               |
 *      +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 *      |     Extended payload length continued, if payload len == 127  |
 *      + - - - - - - - - - - - - - - - +-------------------------------+
 *      |                               |Masking-key, if MASK set to 1  |
 *      +-------------------------------+-------------------------------+
 *      | Masking-key (continued)       |          Payload Data         |
 *      +-------------------------------- - - - - - - - - - - - - - - - +
 *      :                     Payload Data continued ...                :
 *      + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 *      |                     Payload Data continued ...                |
 *      +---------------------------------------------------------------+
 */
/* LSB = Least significant bit
 * MSB = Most significant bit
 */
typedef struct 
{
    /* ------ byte 0 ----- */
    // LSB
    u8 opcode: 4; /* Payload interpretation: text, binary, control frame, continuation frame */

    /* Reserved. Must all be 0s */
    u8 rsv3: 1;
    u8 rsv2: 1;
    u8 rsv1: 1;

    u8 fin: 1; /* Final fragment */
    // MSB
    /* ------ byte 0 ----- */


    /* ------ byte 1 ----- */
    // LSB
    u8 payload_len: 7; /* Payload len. 7 bits, 16 bits or 64 bits */
    u8 mask: 1; /* If payload is masked */
    // MSB
    /* ------ byte 1 ----- */
} ws_frame_t;

#define WS_MASKKEY_LEN 4

typedef struct 
{
    union {
        u16 u16;
        u64 u64;
    };
    u8 maskkey[WS_MASKKEY_LEN];
} ws_frame_ext_t;

typedef struct 
{
    ws_frame_t frame;
    ws_frame_ext_t ext;

    size_t payload_len;
    char* payload;
} ws_t;

enum client_recv_status server_ws_parse(server_thread_t* th, client_t* client, u8* buf, size_t buf_len);
ssize_t ws_send(client_t* client, const char* buf, size_t len);
ssize_t ws_send_adv(client_t* client, u8 opcode, const char* buf, size_t len, const u8* maskkey);
ssize_t ws_json_send(client_t* client, json_object* json);

#endif // _SERVER_WEBSOCKET_H_
