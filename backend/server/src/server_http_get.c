#include "server.h"
#include "server_http.h"
#include "server_util.h"

static inline void
server_handle_set_session(server_t* server, client_t* client, http_t* http)
{
    const char* token = NULL;

    for (u32 i = 0; i < http->n_params; i++)
    {
        const http_header_t* param = http->params + i;
        if (strcmp(param->name, "token") == 0)
        {
            token = param->val;
            break;
        }
    }

    if (token == NULL)
    {
        error("No token provided\n");
        server_http_resp_error(client, HTTP_CODE_BAD_REQ);
        return;
    }

    u64 tokenid = strtoull(token, NULL, 10);
    client_t* ws_client = server_ght_get(&server->client_by_tmptoken_ht, tokenid);
    if (ws_client)
    {
        http_t* resp_http = http_new_resp(HTTP_CODE_OK, NULL, 0);
        char* set_cookie = http_add_header(resp_http, "Set-Cookie", NULL);
        if (set_cookie == NULL)
        {
            warn("http_add_header() returned NULL!\n");
            return;
        }
        snprintf(set_cookie, HTTP_HEAD_VAL_LEN - 1, "session_id=%s; HttpOnly; Secure; SameSite=Strict", ws_client->session_uuid);
        http_send(client, resp_http);
        server_ght_del(&server->client_by_tmptoken_ht, tokenid);
        http_free(resp_http);
    }
    else
    {
        error("No client in tmptoken_ht\n");
        server_http_resp_error(client, HTTP_CODE_UNAUTHORIZED);
    }
}

enum client_recv_status 
server_handle_http_get(server_t* server, client_t* client, http_t* http)
{    
    char path[PATH_MAX];
    memset(path, 0, PATH_MAX);
    i32 fd;
    size_t content_len;
    char* content;
    size_t url_len = strnlen(http->req.url, HTTP_URL_LEN);
    bool notfound = false;

    if (strcmp(http->req.url, "/set-session") == 0)
    {
        server_handle_set_session(server, client, http);
        return RECV_OK;
    }

    if (server_http_url_checks(http) == -1)
    {
        server_http_resp_error(client, HTTP_CODE_NOT_FOUND);
        return RECV_ERROR;
    }

    // TODO: Add "default_file_per_dir" config, default should be "index.html"
format_path:
    if (http->req.url[url_len - 1] == '/')
        snprintf(path, PATH_MAX, "%s%s%s", server->conf.root_dir, http->req.url, "index.html"); 
    else
        snprintf(path, PATH_MAX, "%s%s", server->conf.root_dir, http->req.url); 
    
    i32 isdir = file_isdir(path);
    if (isdir == -1)
    {
        if (notfound)
        {
            server_http_resp_404_not_found(client);
            return RECV_ERROR;
        }

        strcpy(http->req.url, "/");
        notfound = true;
        goto format_path;
    }
    else if (isdir)
        strcat(path, "/index.html");

    const char* content_type = server_get_content_type(path);

    fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        error("GET request of '%s' failed: %s\n", path, ERRSTR);
        server_http_resp_404_not_found(client);
        return RECV_ERROR;
    }
    content_len = fdsize(fd);
    
    if (!HTTP_CMP_METHOD("HEAD"))
        content = NULL;
    else
    {
        content = malloc(content_len);
        if (read(fd, content, content_len) == -1)
        {
            error("read '%s' failed: %s\n", path, ERRSTR);
            close(fd);
            free(content);
            server_http_resp_404_not_found(client);
            return RECV_ERROR;
        }
    }
    close(fd);

    if (strcmp(content_type, "application/octet-stream") == 0)
    {
        const char* temp = server_mime_type(server, content, content_len);
        if (temp)
            content_type = temp;
    }

    verbose("Got file (%s): '%s'\n", content_type, path);
    server_http_resp_ok(client, content, content_len, content_type);
    free(content);

    return RECV_OK;
}

