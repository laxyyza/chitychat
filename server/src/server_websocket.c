#include "server_websocket.h"
#include "server.h"
#include "chat/ws_text_frame.h"

ssize_t 
server_ws_pong(client_t* client, const void* payload, size_t len)
{
    ssize_t bytes_sent = ws_send_adv(client, 
                                     WS_PONG_FRAME, 
                                     payload, len, 
                                     NULL);

    return bytes_sent;
}

static void 
server_ws_parse_check_offset(const ws_t* ws, size_t* offset)
{
    if (ws->frame.payload_len == 126)
        *offset += 2;
    else if (ws->frame.payload_len == 127)
        *offset += 8;

    if (ws->frame.mask)
        *offset += WS_MASKKEY_LEN;
}

enum client_recv_status 
server_ws_parse(server_thread_t* th, client_t* client, 
                u8* buf, size_t buf_len)
{
    ws_t ws;
    memset(&ws, 0, sizeof(ws_t));
    size_t offset = sizeof(ws_frame_t);
    u8* next_buf = NULL;
    size_t next_size = 0;
    enum client_recv_status ret = RECV_OK;

    memcpy(&ws.frame, buf, sizeof(ws_frame_t));

    // Set the payload offset
    server_ws_parse_check_offset(&ws, &offset);

    if (ws.frame.payload_len == 126)
    {
        ws.ext.u16 = WS_PAYLOAD_LEN16(buf);
        ws.payload_len = (u16)ws.ext.u16;
    }
    else if (ws.frame.payload_len == 127)
    {
        ws.ext.u64 = WS_PAYLOAD_LEN64(buf);
        ws.payload_len = ws.ext.u64;
    }
    else
        ws.payload_len   = ws.frame.payload_len;

    const size_t total_size = ws.payload_len + offset;

    if (total_size < buf_len)
    {
        /*
         * If total packet size is less than received buffer size,
         * that means 2 or more packets in received buffer recv() from client
         * Calculate the pointer to the next packet in the buffer and its size
         */
        next_buf = buf + total_size;
        next_size = buf_len - total_size;
    }
    else if (total_size > buf_len)
    {
        /*
         * If the total packet size is bigger than the recv buffer size
         *
         * resize buffer based on the packet header's payload size,
         * and update client recv information
         */        
        client->recv.data = realloc(client->recv.data, total_size);
        client->recv.data_size = total_size;
        client->recv.offset = buf_len;
        client->recv.busy = true;

        return RECV_OK;
    }

    if (ws.frame.mask)
    {
        // Unmask payload
        memcpy(ws.ext.maskkey, buf + (offset - WS_MASKKEY_LEN), WS_MASKKEY_LEN);
        ws.payload = (char*)&buf[offset];
        mask((u8*)ws.payload, ws.payload_len, ws.ext.maskkey, 
            WS_MASKKEY_LEN);
    }
    else
        ws.payload = (char*)buf + offset;

    switch (ws.frame.opcode)
    {
        case WS_CONTINUE_FRAME:
            warn("Not handled CONTINUE FRAME: %s\n", ws.payload);
            break;
        case WS_TEXT_FRAME:
            ret = server_ws_handle_text_frame(th, client, ws.payload, 
                                        ws.payload_len);
            break;
        case WS_BINARY_FRAME:
            warn("Not handled binary frame: %s\n", ws.payload);
            break;
        case WS_CLOSE_FRAME:
            return RECV_DISCONNECT;
        case WS_PING_FRAME:
            server_ws_pong(client, ws.payload, ws.payload_len);
            break;
        case WS_PONG_FRAME:
            warn("Not handled pong frame: %s\n", ws.payload);
            break;
        default:
            warn("UNKNOWN FRAME (%u): %s\n", ws.frame.opcode, ws.payload);
            break;
    }

    if (next_buf && ret == RECV_OK)
        ret = server_ws_parse(th, client, next_buf, next_size);
    else
        client->recv.busy = false;

    return ret;
}

ssize_t 
ws_send_adv(const client_t* client, u8 opcode, const char* buf, size_t len, 
                    const u8* maskkey) 
{
    ssize_t bytes_sent = 0;
    struct iovec iov[4];
    size_t i = 0;
    ws_t ws = {
        .frame.fin = 1,
        .frame.rsv1 = 0,
        .frame.rsv2 = 0,
        .frame.rsv3 = 0,
        .frame.opcode = opcode,
        .frame.mask = (maskkey) ? 1 : 0,
        .frame.payload_len = len
    };

    iov[i].iov_base = &ws.frame;
    iov[i].iov_len = sizeof(ws_frame_t);

    if (len >= 126)
    {
        i++;
        if (len >= UINT16_MAX)
        {
            verbose("WS SEND 64-bit\n");
            ws.frame.payload_len = 127;
            swpcpy((u8*)&ws.ext.u64, (u8*)&len, sizeof(u64));
            iov[i].iov_base = &ws.ext.u64;
            iov[i].iov_len = sizeof(u64);
        }
        else
        {
            verbose("WS SEND 16-bit\n");
            ws.frame.payload_len = 126;
            swpcpy((u8*)&ws.ext.u16, (const u8*)&len, sizeof(u16));
            iov[i].iov_base = &ws.ext.u16;
            iov[i].iov_len = sizeof(u16);
        }
    }

    if (ws.frame.mask)
    {
        i++;
        iov[i].iov_base = (u8*)maskkey;
        iov[i].iov_len = WS_MASKKEY_LEN;

        if (buf)
            mask((u8*)buf, len, maskkey, WS_MASKKEY_LEN);

    }

    if (buf)
    {
        i++;
        iov[i].iov_base = (char*)buf;
        iov[i].iov_len = len;
    }

    i++;
    size_t buffer_size;
    void* buffer = combine_buffers(iov, i, &buffer_size);

    bytes_sent = server_send(client, buffer, buffer_size);
    free(buffer);

    return bytes_sent;
}

ssize_t 
ws_send(const client_t* client, const char* buf, size_t len)
{
    return ws_send_adv(client, WS_TEXT_FRAME, buf, len, NULL);
}

ssize_t 
ws_json_send(const client_t* client, json_object* json)
{
    size_t len;
    const char* string = json_object_to_json_string_length(json, 0, &len);

    return ws_send(client, string, len);
}
