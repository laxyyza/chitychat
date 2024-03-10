#include "server.h"
#include <signal.h>
#include "server_crypt.h"
#include <sys/random.h>
#include <sys/timerfd.h>

#include <json-c/json.h>

#define MAX_EP_EVENTS 10

bool        server_load_config(server_t* server, const char* config_path)
{
#define JSON_GET(x) json_object_object_get(config, x)

    json_object* config;
    json_object* root_dir;
    json_object* addr_ip;
    json_object* addr_port;
    json_object* addr_version;
    json_object* database;
    json_object* sql_filepaths;

    config = json_object_from_file(config_path);
    if (!config)
    {
        fatal(json_util_get_last_err());
        return false;
    }

    root_dir = JSON_GET("root_dir");
    const char* root_dir_str = json_object_get_string(root_dir);
    strncpy(server->conf.root_dir, root_dir_str, CONFIG_PATH_LEN);

    addr_ip = JSON_GET("addr_ip");
    const char* addr_ip_str = json_object_get_string(addr_ip);
    strncpy(server->conf.addr_ip, addr_ip_str, INET6_ADDRSTRLEN);

    addr_port = JSON_GET("addr_port");
    server->conf.addr_port = json_object_get_int(addr_port);

    addr_version = JSON_GET("addr_version");
    const char* addr_version_str = json_object_get_string(addr_version);
    if (!strcmp(addr_version_str, "ipv4"))
        server->conf.addr_version = IPv4;
    else if (!strcmp(addr_version_str, "ipv6"))
        server->conf.addr_version = IPv6;
    else
        warn("Config: addr_version: \"%s\"? Default to IPv4\n", addr_version_str);

    database = JSON_GET("database");
    const char* database_str = json_object_get_string(database);
    strncpy(server->conf.database, database_str, CONFIG_PATH_LEN);

    json_object_put(config);

    server->conf.sql_schema = "server/sql/schema.sql";
    server->conf.sql_insert_user = "server/sql/insert_user.sql";
    server->conf.sql_select_user = "server/sql/select_user.sql";
    server->conf.sql_delete_user = "server/sql/delete_user.sql";
    server->conf.sql_insert_group = "server/sql/insert_group.sql";
    server->conf.sql_select_group = "server/sql/select_group.sql";
    server->conf.sql_delete_group = "server/sql/delete_group.sql";
    server->conf.sql_insert_groupmember = "server/sql/insert_groupmember.sql";
    server->conf.sql_select_groupmember = "server/sql/select_groupmember.sql";
    server->conf.sql_delete_groupmember = "server/sql/delete_groupmember.sql";
    server->conf.sql_insert_msg = "server/sql/insert_msg.sql";
    server->conf.sql_select_msg = "server/sql/select_msg.sql";
    server->conf.sql_delete_msg = "server/sql/delete_msg.sql";
    server->conf.sql_update_user = "server/sql/update_user.sql";

    return true;
}

static bool server_init_socket(server_t* server)
{
    int domain;

    if (server->conf.addr_version == IPv4)
    {
        domain = AF_INET;
        server->addr_len = sizeof(struct sockaddr_in);
        server->addr_in.sin_family = AF_INET;
        server->addr_in.sin_port = htons(server->conf.addr_port);

        if (!strncmp(server->conf.addr_ip, "any", 3))
            server->addr_in.sin_addr.s_addr = INADDR_ANY;
        else
        {
            server->addr_in.sin_addr.s_addr = inet_addr(server->conf.addr_ip);
            if (server->addr_in.sin_addr.s_addr == INADDR_NONE)
            {
                fatal("Invalid IP in address: '%s'.\n", server->conf.addr_ip);
                return false;
            }
        }
    }
    else
    {
        domain = AF_INET6;
        server->addr_len = sizeof(struct sockaddr_in6);
        server->addr_in6.sin6_family = AF_INET6;
        server->addr_in6.sin6_port = htons(server->conf.addr_port);
        if (!strncmp(server->conf.addr_ip, "any", 3))
            server->addr_in6.sin6_addr = in6addr_any;
        else
        {
            if (inet_pton(AF_INET6, server->conf.addr_ip, &server->addr_in6.sin6_addr) <= 0)
            {
                fatal("inet_pton: Invalid IPv6 address: '%s'\n", server->conf.addr_ip);
                return false;
            }
        }
    }

    server->addr = (struct sockaddr*)&server->addr_in;

    server->sock = socket(domain, SOCK_STREAM, 0);
    if (server->sock == -1)
    {
        fatal("socket: %s\n", strerror(errno));
        return false;
    }

    int opt = 1;
    if (setsockopt(server->sock, SOL_SOCKET, SO_REUSEADDR, &opt, server->addr_len) == -1)
        error("setsockopt: %s\n", strerror(errno));

    if (bind(server->sock, server->addr, server->addr_len) == -1)
    {   
        fatal("bind: %s\n", strerror(errno));
        return false;
    }

    if (listen(server->sock, 10) == -1)
    {
        fatal("listen: %s\n", strerror(errno)); 
        return false;
    }

    return true;
}

i32 server_ep_addfd(server_t* server, i32 fd)
{
    i32 ret;

    struct epoll_event ev = {
        .data.fd = fd,
        .events = EPOLLIN | EPOLLRDHUP | EPOLLHUP
    };

    ret = epoll_ctl(server->epfd, EPOLL_CTL_ADD, fd, &ev);
    if (ret == -1)
        error("epoll_ctl ADD fd:%d: %s\n", fd, ERRSTR);

    return ret;
}

i32 server_ep_delfd(server_t* server, i32 fd)
{
    i32 ret;

    if (!server || !fd)
        return -1;

    ret = epoll_ctl(server->epfd, EPOLL_CTL_DEL, fd, NULL);
    if (ret == -1)
        error("epoll_ctl DEL fd:%d: %s\n", fd, ERRSTR);

    return ret;
}

static bool server_init_epoll(server_t* server)
{
    server->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (server->epfd == -1)
    {
        fatal("epoll_create1: %s\n", ERRSTR);
        return false;
    }
    
    if (server_ep_addfd(server, server->sock) == -1)
        return false;

    return true;
}

static bool server_init_ssl(server_t* server)
{
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    server->ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!server->ssl_ctx)
    {
        error("SSL_CTX_new() returned NULL!\n");
        return false;
    }

    SSL_CTX_set_options(server->ssl_ctx, SSL_OP_SINGLE_DH_USE);
    SSL_CTX_set_ecdh_auto(server->ssl_ctx, 1);
    if (SSL_CTX_use_certificate_file(server->ssl_ctx, "server/server.crt", SSL_FILETYPE_PEM) <= 0)
        error("SSL cert failed: %s\n", ERRSTR);
    if (SSL_CTX_use_PrivateKey_file(server->ssl_ctx, "server/server.key", SSL_FILETYPE_PEM) <= 0)   
        error("SSL private key failed: %s\n", ERRSTR);

    return true;
}

server_t*   server_init(const char* config_path)
{
    server_t* server = calloc(1, sizeof(server_t));

    if (!server_load_config(server, config_path))
        goto error;

    if (!server_init_socket(server))
        goto error;

    if (!server_init_epoll(server))
        goto error;

    if (!server_db_open(server))
        goto error;

    if (!server_init_ssl(server))
        goto error;

    server->running = true;

    return server;
error:;
    server_cleanup(server);
    return NULL;
}

static void server_get_client_info(client_t* client)
{
    i32 ret;
    i32 domain;

    ret = getnameinfo(client->addr.addr_ptr, client->addr.len, client->addr.host, NI_MAXHOST, client->addr.serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
    if (ret == -1)
    {
        error("getnameinfo: %s\n", ERRSTR);
    }

    if (client->addr.version == IPv4)
    {
        domain = AF_INET;
        inet_ntop(domain, &client->addr.ipv4.sin_addr, client->addr.ip_str, INET_ADDRSTRLEN);
    }
    else
    {
        domain = AF_INET6;
        inet_ntop(domain, &client->addr.ipv6.sin6_addr, client->addr.ip_str, INET6_ADDRSTRLEN);
    }

}

static void server_accept_conn(server_t* server)
{
    client_t* client = server_new_client(server);

    client->addr.len = server->addr_len;
    client->addr.version = server->conf.addr_version;
    client->addr.addr_ptr = (struct sockaddr*)&client->addr.ipv4;
    client->addr.sock = accept(server->sock, client->addr.addr_ptr, &client->addr.len);
    if (client->addr.sock == -1)
    {
        error("accept: %s\n", ERRSTR);
        return;
    }

    server_client_ssl_handsake(server, client);

    server_get_client_info(client);
    server_ep_addfd(server, client->addr.sock);

    info("Client (fd:%d, IP: %s:%s, host: %s) connected.\n", 
        client->addr.sock, client->addr.ip_str, client->addr.serv, client->addr.host);
}

static void server_read_fd(server_t* server, const i32 fd, const i32 flags)
{
    ssize_t bytes_recv;
    u8* buf;
    size_t buf_size;
    http_t* http;
    enum client_recv_status recv_status = RECV_OK;
    client_t* client = server_get_client_fd(server, fd);
    if (!client)
    {
        // TODO: Fix missing clients
        error("fd: %d no client. FIX YOUR LINKED LIST!\n", fd);
        server_ep_delfd(server, fd);
        return;
    }

    http = client->recv.http;

    if (http)
    {
        buf = (u8*)http->body + http->buf.total_recv;
        buf_size = http->body_len - http->buf.total_recv;
    }
    else
    {
        if (!client->recv.data)
        {
            client->recv.data = calloc(1, CLIENT_RECV_PAGE);
            client->recv.data_size = CLIENT_RECV_PAGE;
        }
        buf = client->recv.data;
        buf_size = client->recv.data_size;
        memset(buf, 0, buf_size);
    }

    debug("Reading from fd: %d\n", fd);

    bytes_recv = server_recv(client, buf, buf_size);
    // bytes_recv = recv(client->addr.sock, buf, buf_size, flags);
    if (bytes_recv == -1)
    {
        client->recv.n_errors++;
        error("Client (fd:%d, IP: %s:%s) recv #%d error: %s\n", 
            client->addr.sock, client->addr.ip_str, client->recv.n_errors, client->addr.serv);
        if (client->recv.n_errors >= CLIENT_MAX_ERRORS)
            server_del_client(server, client);
        return;
    }
    else if (bytes_recv == 0)
    {
        debug("Client recv 0 bytes, disconnecting...\n");
        server_del_client(server, client);
        return;
    }
    else if (http)
    {
        http->buf.total_recv += bytes_recv;
        debug("HTTP recv: %zu/%zu\n", http->buf.total_recv, http->body_len);
        if (bytes_recv >= buf_size)
        {
            server_handle_http(server, client, client->recv.http);
            client->recv.http = NULL;
        }
    }
    else
    {
        if (client->state & CLIENT_STATE_WEBSOCKET)
            recv_status = server_ws_parse(server, client, buf, bytes_recv);
        else
            recv_status = server_http_parse(server, client, buf, bytes_recv);
    }

    if (recv_status != RECV_DISCONNECT && !client->recv.busy)
    {
        free(client->recv.data);
        client->recv.data = NULL;
        client->recv.data_size = 0;
    }

    if (recv_status == RECV_OK && client->recv.overflow_check != CLIENT_OVERFLOW_CHECK_MAGIC)
    {
        error("Client fd: %d recv overflow check failed! %X != %X\n", client->addr.sock, client->recv.overflow_check, CLIENT_OVERFLOW_CHECK_MAGIC);
    }
}

static void server_ep_event(server_t* server, const struct epoll_event* event)
{
    const i32 fd = event->data.fd;
    const u32 ev = event->events;
    i32 recv_flags = 0;

    if (ev & EPOLLERR)
    {
        i32 error = 0;
        socklen_t errlen = sizeof(i32);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &errlen) == 0)
        {
            error("Epoll error on fd: %d: %s\n", fd, strerror(error));
        }
        else
            error("epoll error on fd: %d, (no error string).\n", fd);
    }

    if (ev & EPOLLPRI)
    {
        error("epoll urgent data on fd: %d\n", fd);
        recv_flags = MSG_OOB;
    }

    if (ev & EPOLLRDHUP || ev & EPOLLHUP)
    {
        client_t* client = server_get_client_fd(server, fd);
        debug("fd: %d hang up.\n", fd);
        if (client)
            server_del_client(server, client);
        else
        {
            warn("fd: %d no client, removing from epoll.\n", fd);
            server_ep_delfd(server, fd);
            close(fd);
        }

        return;
    }
    
    if (fd == server->sock)
    {
        // New connection...
        server_accept_conn(server);
    }
    else if (ev & EPOLLIN)
    {
        // fd ready for read
        server_read_fd(server, fd, recv_flags);
    }
    else
    {
        warn("Not handled fd: %d, ev: %u\n", fd, ev);
    }
}

// Should ONLU be used in server_sig_handler()
server_t* __server_sig = NULL;

static void server_sig_handler(i32 signum)
{
    debug("signal: %d\n", signum);

    __server_sig->running = false;
}

static void server_init_signal_handlers(server_t* server)
{
    struct sigaction sa;
    sa.sa_handler = server_sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    __server_sig = server;

    if (sigaction(SIGINT, &sa, NULL) == -1)
        error("sigaction: %s\n", ERRSTR);
}

void server_run(server_t* server)
{
    i32 nfds;
    struct epoll_event events[MAX_EP_EVENTS];

    server_init_signal_handlers(server);

    info("Server listening on IP: %s, port: %u\n", server->conf.addr_ip, server->conf.addr_port);

    while (server->running)
    {
        if ((nfds = epoll_wait(server->epfd, events, MAX_EP_EVENTS, -1)) == -1)
        {
            error("epoll_wait: %s\n", ERRSTR);
            server->running = false;
        }

        for (i32 i = 0; i < nfds; i++)
            server_ep_event(server, events + i);
    }
}

static void server_del_all_clients(server_t* server)
{
    client_t* node = server->client_head;
    client_t* next = NULL;

    while (node)
    {
        next = node->next;
        server_del_client(server, node);
        node = next;
    }
}

static void server_del_all_sessions(server_t* server)
{
    session_t* node;
    session_t* next;

    node = server->session_head;

    while (node)
    {
        next = node->next;
        server_del_client_session(server, node);
        node = next;
    }
}

static void server_del_all_upload_tokens(server_t* server)
{
    upload_token_t* node;
    upload_token_t* next;

    node = server->upload_token_head;

    while (node)
    {
        next = node->next;
        debug("Deleting upload token: %u for user %u\n", node->token, node->user_id);
        server_del_upload_token(server, node);
        node = next;
    }
}

void server_cleanup(server_t* server)
{
    if (!server)
        return;

    server_del_all_clients(server);
    server_del_all_sessions(server);
    server_del_all_upload_tokens(server);
    server_db_close(server);

    SSL_CTX_free(server->ssl_ctx);

    if (server->epfd)
        close(server->epfd);
    if (server->sock)
        close(server->sock);

    free(server);
}

ssize_t server_send(const client_t* client, const void* buf, size_t len)
{
    ssize_t bytes_sent;

    if (client->secure)
        bytes_sent = SSL_write(client->ssl, buf, len);
    else
        bytes_sent = send(client->addr.sock, buf, len, 0);

    return bytes_sent;
}

ssize_t server_recv(const client_t* client, void* buf, size_t len)
{
    ssize_t bytes_recv;

    if (client->secure)
        bytes_recv = SSL_read(client->ssl, buf, len);
    else
        bytes_recv = recv(client->addr.sock, buf, len, 0);

    return bytes_recv;
}
 
server_event_t* server_new_event(server_t* server, i32 fd, void* data, 
            se_read_callback_t read_callback, se_close_callback_t close_callback)
{
    server_event_t* se;
    server_event_t* head_next;

    if (!server || !data || !read_callback || !close_callback)
    {
        warn("server_new_event(%p, %d, %p, %p, %p): Something is NULL!\n",
                server, fd, data, read_callback, close_callback);
        return NULL;
    }
    
    se = calloc(1, sizeof(server_event_t));
    if (server_ep_addfd(server, fd) == -1)
    {
        free(se);
        error("ep_addfd %d failed\n", fd);
        return NULL;
    }
    se->fd = fd;
    se->data = data;
    se->read = read_callback;
    se->close = close_callback;

    if (server->se_head == NULL)
    {
        server->se_head = se;
        return se;
    }

    head_next = server->se_head->next;
    if (head_next)
        head_next->prev = se;
    server->se_head->next = se;
    se->next = head_next;
    se->prev = server->se_head;

    return se;
}

void server_del_event(server_t* server, server_event_t* se)
{
    server_event_t* next;
    server_event_t* prev;

    if (!server || !se)
    {
        warn("server_del_event(%p, %p): Something is NULL!\n",
                server, se);
        return;
    }

    server_ep_delfd(server, se->fd);
    if (se->close)
        se->close(server, se->data);
    else
        if (close(se->fd) == -1)
            error("close(%d): %s\n", se->fd, ERRSTR);

    prev = se->prev;
    next = se->next;
    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;
    if (se == server->se_head)
        server->se_head = next;

    free(se);
}

server_event_t* server_get_event(server_t* server, i32 fd)
{
    server_event_t* node;

    node = server->se_head;

    while (node)
    {
        if (node->fd == fd)
            return node;
        node = node->next;
    }

    return NULL;
}

session_t* server_new_client_session(server_t* server, client_t* client)
{
    session_t* new_sesion;
    session_t* head_next;

    if (!server || !client || !client->dbuser)
    {
        warn("new_client_session(%p, %p) dbuser: %p: Something is NULL!\n", server, client, client->dbuser);
        return NULL;
    }

    new_sesion = calloc(1, sizeof(session_t));
    getrandom(&new_sesion->session_id, sizeof(u32), 0);
    new_sesion->user_id = client->dbuser->user_id;

    client->session = new_sesion;

    if (server->session_head == NULL)
    {
        server->session_head = new_sesion;
        return new_sesion;
    }

    head_next = server->session_head->next;
    server->session_head->next = new_sesion;
    new_sesion->next = head_next;
    if (head_next)
        head_next->prev = new_sesion;
    new_sesion->prev = server->session_head;

    return new_sesion;
}

session_t* server_get_client_session(server_t* server, u32 session_id)
{
    session_t* node;

    node = server->session_head;

    while (node)
    {
        if (node->session_id == session_id)
            return node;
        node = node->next;
    }

    return NULL;
}

void server_del_client_session(server_t* server, session_t* session)
{
    session_t* next;
    session_t* prev;

    if (!server || !session)
    {
        warn("del_client_session(%p, %p): Something is NULL!\n", server, session);
        return;
    }

    debug("Deleting session: %u for user %u\n", session->session_id, session->user_id);

    if (session->timerfd)
    {
        server_ep_delfd(server, session->timerfd);
        if (close(session->timerfd) == -1)
            error("close(%d) session timerfd: %s\n", session->timerfd, ERRSTR);
    }

    next = session->next;
    prev = session->prev;

    if (next)
        next->prev = prev;
    if (prev)
        prev->next = next;
    if (session == server->session_head)
        server->session_head = next;

    free(session);
}

upload_token_t* server_new_upload_token(server_t* server, u32 user_id)
{
    upload_token_t* ut;
    upload_token_t* head_next;

    ut = calloc(1, sizeof(upload_token_t));
    ut->user_id = user_id;
    getrandom(&ut->token, sizeof(u32), 0);

    if (server->upload_token_head == NULL)
    {
        server->upload_token_head = ut;
        return ut;
    }

    head_next = server->upload_token_head->next;
    server->upload_token_head->next = ut;
    ut->next = head_next;
    ut->prev = server->upload_token_head;
    if (head_next)
        head_next->prev = ut;

    return ut;
}

upload_token_t* server_get_upload_token(server_t* server, u32 token)
{
    upload_token_t* node;

    node = server->upload_token_head;

    while (node) 
    {
        if (node->token == token)
            return node;
        node = node->next;
    }

    return NULL;
}

void server_del_upload_token(server_t* server, upload_token_t* upload_token)
{
    upload_token_t* next;
    upload_token_t* prev;

    if (!server || !upload_token)
    {
        warn("del_upload_token(%p, %p): Something is NULL!\n", server, upload_token);
        return;
    }

    if (upload_token->timerfd)
    {
        server_ep_delfd(server, upload_token->timerfd);
        if (close(upload_token->timerfd) == -1)
            error("close(%d) ut timerfd: %s\n", upload_token->timerfd, ERRSTR);
    }

    next = upload_token->next;
    prev = upload_token->prev;
    if (next)
        next->prev = prev;
    if (prev)
        prev->next = next;
    if (upload_token == server->upload_token_head)
        server->upload_token_head = next;

    free(upload_token);
}