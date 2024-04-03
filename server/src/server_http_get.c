#include "server.h"
#include "server_http.h"
#include "server_util.h"

enum client_recv_status 
server_handle_http_get(server_t* server, client_t* client, http_t* http)
{    
    char path[PATH_MAX];
    memset(path, 0, PATH_MAX);
    i32 fd;
    size_t content_len;
    char* content;
    size_t url_len = strnlen(http->req.url, HTTP_URL_LEN);

    if (server_http_url_checks(http) == -1)
    {
        server_http_resp_error(client, HTTP_CODE_NOT_FOUND, "Not found");
        return RECV_ERROR;
    }

    // TODO: Add "default_file_per_dir" config, default should be "index.html"
    if (http->req.url[url_len - 1] == '/')
        snprintf(path, PATH_MAX, "%s%s%s", server->conf.root_dir, http->req.url, "index.html"); 
    else
        snprintf(path, PATH_MAX, "%s%s", server->conf.root_dir, http->req.url); 
    
    i32 isdir = file_isdir(path);
    if (isdir == -1)
    {
        server_http_resp_404_not_found(client);
        return RECV_ERROR;
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
    content = malloc(content_len);
    if (read(fd, content, content_len) == -1)
    {
        error("read '%s' failed: %s\n", path, ERRSTR);
        close(fd);
        server_http_resp_404_not_found(client);
        return RECV_ERROR;
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

