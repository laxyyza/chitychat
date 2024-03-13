#include "server_websocket.h"
#include "server.h"
#include "server_ws_pld_hdlr.h"
#include <sys/uio.h>

ssize_t server_ws_pong(client_t* client)
{
    ssize_t bytes_sent = ws_send_adv(client, 
            WS_PING_FRAME, NULL, 0, NULL);

    return bytes_sent;
}

static void server_ws_parse_check_offset(const ws_t* ws, size_t* offset)
{

    if (ws->frame.payload_len == 126)
        *offset += 2;
    else if (ws->frame.payload_len == 127)
        *offset += 8;

    if (ws->frame.mask)
        *offset += WS_MASKKEY_LEN;
}

enum client_recv_status server_ws_parse(server_t* server, 
            client_t* client, u8* buf, size_t buf_len)
{
    ws_t ws;
    memset(&ws, 0, sizeof(ws_t));
    size_t offset = sizeof(ws_frame_t);
    u8* next_buf = NULL;
    size_t next_size = 0;
    enum client_recv_status ret = RECV_OK;

    memcpy(&ws.frame, buf, sizeof(ws_frame_t));

    verbose("FIN: %u\n", ws.frame.fin);
    verbose("RSV1: %u\n", ws.frame.rsv1);
    verbose("RSV2: %u\n", ws.frame.rsv2);
    verbose("RSV3: %u\n", ws.frame.rsv3);
    verbose("MASK: %u\n", ws.frame.mask);
    verbose("OPCODE: %02X\n", ws.frame.opcode);
    verbose("PAYLOAD LEN: %u\n", ws.frame.payload_len);

    server_ws_parse_check_offset(&ws, &offset);

    if (ws.frame.payload_len == 126)
    {
        ws.ext.u16 = WS_PAYLOAD_LEN16(buf);
        ws.payload_len = (u16)ws.ext.u16;
        // offset += 2;
        verbose("Payload len 16-bit: %zu\n", ws.payload_len);
    }
    else if (ws.frame.payload_len == 127)
    {
        ws.ext.u64 = WS_PAYLOAD_LEN64(buf);
        ws.payload_len = ws.ext.u64;
        // offset += 8;
        verbose("Payload len 64-bit\n");
    }
    else
        ws.payload_len   = ws.frame.payload_len;

    const size_t total_size = ws.payload_len + offset;
    if (total_size < buf_len)
    {
        next_buf = buf + total_size;
        next_size = buf_len - total_size;
    }
    else if (total_size > buf_len)
    {
        u8* new_buf = malloc(total_size);
        size_t new_offset = buf_len;
        memcpy(new_buf, buf, buf_len);
        client->recv.busy = true;

        free(client->recv.data);
        client->recv.data = new_buf;
        client->recv.data_size = total_size;
        client->recv.offset = new_offset;

        return RECV_OK;
    }

    if (ws.frame.mask)
    {
        memcpy(ws.ext.maskkey, buf + (offset - WS_MASKKEY_LEN), WS_MASKKEY_LEN);
        // offset += WS_MASKKEY_LEN;
        ws.payload = (char*)&buf[offset];
        mask((u8*)ws.payload, ws.payload_len, ws.ext.maskkey, 
            WS_MASKKEY_LEN);
    }
    else
        ws.payload = (char*)buf + offset;

    switch (ws.frame.opcode)
    {
        case WS_CONTINUE_FRAME:
            verbose("CONTINUE FRAME: %s\n", ws.payload);
            break;
        case WS_TEXT_FRAME:
            ret = server_ws_handle_text_frame(server, client, ws.payload, 
                                        ws.payload_len);
            break;
        case WS_BINARY_FRAME:
            verbose("BINARY FRAME: %s\n", ws.payload);
            break;
        case WS_CLOSE_FRAME:
        {
            verbose("CLOSE FRAME: %s\n", ws.payload);
            server_del_client(server, client);
            return RECV_DISCONNECT;
        }
        case WS_PING_FRAME:
        {
            verbose("PING FRAME: %s\n", ws.payload);
            server_ws_pong(client);
            break;
        }
        case WS_PONG_FRAME:
            verbose("PONG FRAME: %s\n", ws.payload);
            break;
        default:
            verbose("UNKNOWN FRAME (%u): %s\n", ws.frame.opcode, ws.payload);
            break;
    }

    if (next_buf && ret == RECV_OK)
    {
        verbose("next_buf: %p, size: %zu\n", next_buf, next_size);
        ret = server_ws_parse(server, client, next_buf, next_size);
    }
    else
        client->recv.busy = false;

    return ret;
}

void* combine_buffers(struct iovec* iov, size_t n, size_t* size_ptr)
{
    size_t size = 0;
    void* buffer;
    size_t index = 0;

    for (size_t i = 0; i < n; i++)
        size += iov[i].iov_len;

    buffer = calloc(1, size);
    for (size_t i = 0; i < n; i++)
    {
        const size_t iov_len = iov[i].iov_len;
        memcpy(buffer + index, iov[i].iov_base, iov_len);
        index += iov_len;
    }

    *size_ptr = size;

    return buffer;
}

ssize_t ws_send_adv(const client_t* client, u8 opcode, const char* buf, size_t len, 
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

    if (len >= 125)
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

    if ((bytes_sent = server_send(client, buffer, buffer_size)) == -1)
    {
        error("WS Send to (fd:%d, IP: %s:%s): %s\n",
            client->addr.sock, client->addr.ip_str, client->addr.serv, ERRSTR);
    }
    free(buffer);

    return bytes_sent;
}

ssize_t ws_send(const client_t* client, const char* buf, size_t len)
{
    return ws_send_adv(client, WS_TEXT_FRAME, buf, len, NULL);
}