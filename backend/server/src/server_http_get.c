#include "server.h"
#include "server_http.h"
#include "server_util.h"

#define ACCEPT_ENCODING_PLAIN   0x00
#define ACCEPT_ENCODING_GZIP    0x01
#define ACCEPT_ENCODING_BROTLI  0x02
#define ACCEPT_ENCODING_ZSTD    0x04

static inline i32 
try_path(char og_path[PATH_MAX], const char* ext)
{
    i32 fd;
    char path[PATH_MAX + 1];
    snprintf(path, PATH_MAX, "%s%s", og_path, ext);

    fd = open(path, O_RDONLY);

    return fd;
}

static inline i32
get_fd_encoding(char default_path[PATH_MAX], const char** content_type, u32 accept_encoding, u32* accepted_encoding)
{
    i32 fd;
    *content_type = server_get_content_type(default_path);

    if (accept_encoding & ACCEPT_ENCODING_BROTLI)
    {
        if ((fd = try_path(default_path, ".br")) != -1)
        {
            *accepted_encoding = ACCEPT_ENCODING_BROTLI;
            return fd;
        }
    }

    if (accept_encoding & ACCEPT_ENCODING_GZIP)
    {
        if ((fd = try_path(default_path, ".gz")) != -1)
        {
            *accepted_encoding = ACCEPT_ENCODING_GZIP;
            return fd;
        }
    }

    if (accept_encoding & ACCEPT_ENCODING_ZSTD)
    {
        if ((fd = try_path(default_path, ".zst")) != -1)
        {
            *accepted_encoding = ACCEPT_ENCODING_ZSTD;
            return fd;
        }
    }

    return open(default_path, O_RDONLY);
}

static inline i32
http_get_fd(server_t* server, http_t* http, const char** content_type, u32 accept_encoding, u32* accepted_encoding)
{
    char path[PATH_MAX];
    size_t url_len = strnlen(http->req.url, HTTP_URL_LEN);
    bool notfound = false;

format_path:
    if (http->req.url[url_len - 1] == '/')
        snprintf(path, PATH_MAX, "%s%s%s", server->conf.root_dir, http->req.url, "index.html"); 
    else
        snprintf(path, PATH_MAX, "%s%s", server->conf.root_dir, http->req.url); 
    
    i32 isdir = file_isdir(path);
    if (isdir == -1)
    {
        if (notfound)
            return -1;

        strcpy(http->req.url, "/");
        notfound = true;
        goto format_path;
    }
    else if (isdir)
        strcat(path, "/index.html");

    return get_fd_encoding(path, content_type, accept_encoding, accepted_encoding);
}

static inline u32 
get_accept_encoding_flags(http_t* http)
{
    u32 ret = ACCEPT_ENCODING_PLAIN;
    http_header_t* accept_encoding_header = http_get_header(http, "accept-encoding");
    char* endptr;
    char* token;
    if (accept_encoding_header == NULL)
        return ret;
    char* accept_encoding = accept_encoding_header->val;

    token = strtok_r(accept_encoding, ", ", &endptr);
    while (token)
    {
        if (strcmp(token, "gzip") == 0)
            ret |= ACCEPT_ENCODING_GZIP;
        else if (strcmp(token, "br") == 0)
            ret |= ACCEPT_ENCODING_BROTLI;
        else if (strcmp(token, "zstd") == 0)
            ret |= ACCEPT_ENCODING_ZSTD;

        token = strtok_r(NULL, ", ", &endptr);
    }

    return ret;
}

static inline void*
http_get_file_content(i32 fd, u32 content_len)
{
    void* ret = NULL;

    ret = malloc(content_len);
    if (read(fd, ret, content_len) == -1)
    {
        error("read: %s\n", ERRSTR);
        free(ret);
        return NULL;
    }

    return ret;
}

enum client_recv_status 
server_handle_http_get(server_t* server, client_t* client, http_t* http)
{    
    i32 fd;
    u32 content_len;
    void* content;
    u32 accept_encoding;
    u32 encoding = ACCEPT_ENCODING_PLAIN;
    http_t* resp;
    const char* content_type;

    if (server_http_url_checks(http) == -1)
    {
        server_http_resp(client, HTTP_CODE_NOT_FOUND);
        return RECV_ERROR;
    }

    accept_encoding = get_accept_encoding_flags(http);
    if ((fd = http_get_fd(server, http, &content_type, accept_encoding, &encoding)) == -1)
    {
        server_http_resp_404_not_found(client);
        return RECV_DISCONNECT;
    }
    content_len = fdsize(fd);
    if (strcmp(http->req.method, "HEAD") == 0)
        content = NULL;
    else
        content = http_get_file_content(fd, content_len);

    resp = http_new_resp(HTTP_CODE_OK, content, content_len);
    if (encoding == ACCEPT_ENCODING_PLAIN)
    {
        if (strcmp(content_type, "application/octet-stream") == 0)
        {
            const char* temp = server_mime_type(server, content, content_len);
            if (temp)
                content_type = temp;
        }
    }
    else if (encoding == ACCEPT_ENCODING_BROTLI)
        http_add_header(resp, "Content-Encoding", "br");
    else if (encoding == ACCEPT_ENCODING_GZIP)
        http_add_header(resp, "Content-Encoding", "gzip");
    else if (encoding == ACCEPT_ENCODING_ZSTD)
        http_add_header(resp, "Content-Encoding", "zstd");
    http_add_header(resp, "Content-Type", content_type);

    http_send(client,resp);
    http_free(resp);
    free(content);
    close(fd);

    return RECV_OK;
}

