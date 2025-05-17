#include "server_init.h"
#include "db/db_def.h"
#include "server.h"
#include "server_events.h"
#include "server_ht.h"
#include "chat/cmd.h"
#include "server_log.h"
#include "server_util.h"
#include <sys/eventfd.h>
#include "backend.h"
#include <sys/resource.h>

#define REDIS_DEFAULT_PORT "6379"

static void
print_help(const char* exe_path)
{
    printf(
        "Usage\n"\
        "\t%s [options]\n"\
        "Chity Chat server\n\n"\
        "  -h, --help\t\t\tShow this message\n"\
        "  -v, --verbose\t\t\tSet log level to verbose\n"\
        "  -d, --debug\t\t\tSet log level to debug\n"\
        "  -p, --port=PORT\t\tPort number to bind\n"\
        "  -T, --thread-pool=N\t\tSet the number of threads for the thread pool,\n"\
        "\t\t\t\tUse -1 (default) to automatically determine the number based on system threads.\n"\
        "  -6, --ipv6\t\t\tUse IPv6\n"\
        "  -4, --ipv4\t\t\tUse IPv4\n"\
        "  -S, --show-sql-times\t\tDisplay SQL query durations in milliseconds.\n"\
        "  -E, --show-event-times\tDisplay event processing durations.\n"\
        "  -t, --show-times\t\tEnable both SQL and event timing displays.\n\n",
        exe_path
    );
}

static bool 
server_argv(server_t* server, int argc, char* const* argv)
{
    i32 opt;
    char* endptr;

    struct option long_opts[] = {
        {"port", required_argument, NULL, 'p'},
        {"verbose", 0, NULL, 'v'},
        {"debug", 0, NULL, 'd'},
        {"ipv6", 0, NULL, '6'},
        {"ipv4", 0, NULL, '4'},
        {"fork", 0, NULL, 'f'},
        {"help", 0, NULL, 'h'},
        {"thread-pool", required_argument, NULL, 'T'},
        {"retry-db-connect", 0, NULL, 'R'},
        {"disable-tls", 0, NULL, 'D'},
        {"show-sql-times", 0, NULL, 'S'},
        {"show-event-times", 0, NULL, 'E'},
        {"show-times", 0, NULL, 't'},
        {NULL, 0, NULL, 0}
    };

    while ((opt = getopt_long(argc, argv, "T:p:v46hfRDSEdt", long_opts, NULL)) != -1)
    {
        switch (opt)
        {
            case 'p':
            {
                u32 port = strtoul(optarg, &endptr, 10);
                if (port == 0)
                {
                    error("Invalid port: '%s'\n", optarg);
                    return false;
                }
                else if (port > UINT16_MAX)
                {
                    error("Port '%u' too large (> %u)\n", port, UINT16_MAX);
                    return false;
                }
                server->conf.addr_port = port;
                break;
            }
            case 'v':
                server_set_loglevel(SERVER_VERBOSE);
                break;
            case 'd':
                server_set_loglevel(SERVER_DEBUG);
                break;
            case 't':
                server->conf.event_time = server->conf.sql_time = true;
                break;
            case '4':
                server->conf.addr_version = IPv4;
                break;
            case '6':
                server->conf.addr_version = IPv6;
                break;
            case 'h':
                print_help(argv[0]);
                return false;
            case 'f':
                server->conf.fork = true;
                break;
            case 'T':
                server->conf.thread_pool = atoi(optarg);
                break;
            case 'R':
                server->conf.retry_db_connect = true;
                break;
            case 'D':
                server->conf.disable_tls = true;
                break;
            case 'S':
                server->conf.sql_time = true;
                break;
            case 'E':
                server->conf.event_time = true;
                break;
            case '?':
                error("Unknown or missing argument\n");
                return false;
        }
    }

    return true;
}

static bool        
server_load_config(server_t* server, int argc, char* const* argv)
{
#define JSON_GET(x) json_object_object_get(config, x)

    const char* root_dir_str;
    const char* img_dir_str;
    const char* vid_dir_str;
    const char* file_dir_str;
    const char* addr_ip_str;
    const char* addr_version_str;
    const char* loglevel_str;
    const char* thread_pool_str;
    const char* port_str;
    const char* disable_tls_str;
    const char* redis_port_str;
    i32 port;
    enum server_log_level log_level = SERVER_DEBUG;

    root_dir_str = getenvd("APP_ROOT_DIR", "client/public");
    strncpy(server->conf.root_dir, root_dir_str, CONFIG_PATH_LEN - 1);

    img_dir_str = getenvd("APP_IMG_DIR", "client/public/imgs");
    strncpy(server->conf.img_dir, img_dir_str, CONFIG_PATH_LEN - 1);

    vid_dir_str = getenvd("APP_VID_DIR", "client/piblic/upload/vids");
    strncpy(server->conf.vid_dir, vid_dir_str, CONFIG_PATH_LEN - 1);

    file_dir_str = getenvd("APP_FILE_DIR", "client/public/upload/files");
    strncpy(server->conf.file_dir, file_dir_str, CONFIG_PATH_LEN - 1);

    addr_ip_str = getenvd("APP_IP", "any");
    strncpy(server->conf.addr_ip, addr_ip_str, INET6_ADDRSTRLEN - 1);

    port_str = getenvd("APP_PORT", "8080");
    port = atoi(port_str);
    if (port <= 0 && port >= UINT16_MAX)
    {
        fatal("Invalid APP_PORT!\n");
        return false;
    }
    server->conf.addr_port = port;

    disable_tls_str = getenvd("APP_DISABLE_TLS", "0");
    if (strcmp(disable_tls_str, "1") == 0 || strcmp(disable_tls_str, "true") == 0)
        server->conf.disable_tls = true;
    else
        server->conf.disable_tls = false;

    addr_version_str = getenvd("APP_IP_VERSION", "ipv6");
    if (!strcmp(addr_version_str, "ipv4"))
        server->conf.addr_version = IPv4;
    else if (!strcmp(addr_version_str, "ipv6"))
        server->conf.addr_version = IPv6;
    else
        warn("Config: addr_version: \"%s\"? Default to IPv4\n", addr_version_str);

    thread_pool_str = getenvd("APP_THREAD_POOL", "0");
    server->conf.thread_pool = atoi(thread_pool_str);

    loglevel_str = getenvd("APP_LOG_LEVEL", "info");

    redis_port_str = getenvd("CC_REDIS_PORT", REDIS_DEFAULT_PORT);
    server->conf.redis_port = atoi(redis_port_str);
    if (server->conf.redis_port <= 0 && server->conf.redis_port >= UINT16_MAX)
    {
        fatal("Invalid CC_REDIS_PORT: %s\n", redis_port_str);
        return false;
    }
    server->conf.redis_ip = getenvd("CC_REDIS_IP", "127.0.0.1");

    if (!strcmp(loglevel_str, "fatal"))
        log_level = SERVER_FATAL;
    else if (!strcmp(loglevel_str, "error"))
        log_level = SERVER_ERROR;
    else if (!strcmp(loglevel_str, "warn") || !strcmp(loglevel_str, "warning"))
        log_level = SERVER_WARN;
    else if (!strcmp(loglevel_str, "info"))
        log_level = SERVER_INFO;
    else if (!strcmp(loglevel_str, "debug"))
        log_level = SERVER_DEBUG;
    else if (!strcmp(loglevel_str, "verbose"))
        log_level = SERVER_VERBOSE;
    else
    {
        fatal("Invalid APP_LOG_LEVEL: %s\n", loglevel_str);
        return false;
    }

    server_set_loglevel(log_level);

    if (!server_argv(server, argc, argv))
        return false;

    verbose("Setting log level: %d\n", log_level);

    if (server->conf.thread_pool == 0)
        server->conf.thread_pool = server_tm_system_threads();

    return true;
}

static bool 
server_set_address(server_t* server)
{
    if (server->conf.addr_version == IPv4)
    {
        server->domain = AF_INET;
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
        server->domain = AF_INET6;
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

    return true;
}

static bool 
server_init_ssl(server_t* server)
{
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_SSL_strings();
    OpenSSL_add_all_algorithms();

    server->ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!server->ssl_ctx)
    {
        error("SSL_CTX_new() returned NULL!\n");
        return false;
    }

    SSL_CTX_set_options(server->ssl_ctx, SSL_OP_SINGLE_DH_USE);
    SSL_CTX_set_ecdh_auto(server->ssl_ctx, 1);
    if (SSL_CTX_use_certificate_file(server->ssl_ctx, "backend/server/server.crt", SSL_FILETYPE_PEM) <= 0)
    {
        error("SSL cert failed: %s\n", ERRSTR);
        return false;
    }
    if (SSL_CTX_use_PrivateKey_file(server->ssl_ctx, "backend/server/server.key", SSL_FILETYPE_PEM) <= 0)   
    {
        error("SSL private key failed: %s\n", ERRSTR);
        return false;
    }

    return true;
}

static bool 
server_init_ht(server_t* server)
{
    const size_t ht_size = 10;

    if (server_ght_init(&server->client_ht, ht_size, NULL) == false)
        return false;

    if (server_ght_init(&server->user_ht, ht_size, (ght_free_t)server_free_user) == false)
        return false;

    if (server_init_chatcmd(server) == false)
        return false;

    return true;
}

static enum se_status
eventfd_read_cb(UNUSED eworker_t* ew, server_event_t* ev)
{
    i64 r;

    if (read(ev->fd, &r, sizeof(i64)) == -1) 
        error("eventfd_read: %s\n", ERRSTR);
    else
        eventfd_write(ev->fd, 1);

    return SE_OK;
}

static bool
server_init_eventfd(eworker_t* ew)
{
    server_t* server = ew->server;

    i32 efd = eventfd(0, 0);
    if (efd == -1)
    {
        fatal("eventfd: %s\n", ERRSTR);
        return false;
    }

    add_event_args_t args = {
        .fd = efd,
        .data = NULL,
        .read_cb = eventfd_read_cb,
        .write_cb = NULL,
        .close_cb = NULL,
        .name = "eventfd",
        .type = FD_SHARED
    };
    server->eventfd = server_epoll_add_event(ew, &args);

    return server->eventfd != NULL;
}

static inline bool 
server_wait_for_workers(server_t* server)
{
    i32 online_workers;
    hr_time_t start_time, current_time;
    nano_gettime(&start_time);
    i32 wait_seconds = 30;

    // busy wait
    while ((online_workers = atomic_load(&server->tm.online_workers)) < server->tm.n_workers)
    {
        nano_gettime(&current_time);
        if (nano_time_diff_s(&start_time, &current_time) >= wait_seconds)
        {
            fatal("Wait time for online workers timed out. %d/%d online workers.\n", 
                  online_workers, server->tm.n_workers);
            return false;
        }
    }
    return true;
}

static inline void 
server_set_rlimit()
{
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) == -1)
    {
        error("getrlimit: %s\n", ERRSTR);
        return;
    }

    if (limit.rlim_cur == limit.rlim_max)
        return;

    limit.rlim_cur = limit.rlim_max;

    if (setrlimit(RLIMIT_NOFILE, &limit) != 0)
        warn("setrlimit: %s\n", ERRSTR);
    else
        debug("Max Open FDs: %d\n", limit.rlim_cur);
}

server_t*   
server_init(int argc, char* const* argv)
{
    server_t* server;

    server = calloc(1, sizeof(server_t));
    if (!server)
    {
        fatal("calloc() failed.\n");
        return NULL;
    }

    // Load config and command line arguments
    if (!server_load_config(server, argc, argv))
        goto error;

    if (!server_set_address(server))
        goto error;

    server_set_rlimit();

    // Init Hash Tables %
    if (!server_init_ht(server))
        goto error;

    // Init DataBase
    if (!server_init_db(server))
        goto error;

    // Init libmagic (for file mime types)
    if (!server_init_magic(server))
        goto error;

    // Init OpenSSL
    if (server->conf.disable_tls == false)
    {
        if (!server_init_ssl(server))
            goto error;
    }

    // Init Thread Manager
    if (!server_init_tm(server, server->conf.thread_pool))
        goto error;

    if (!server_eworker_init(server->main_ew))
        goto error;

    if (!server_init_nats(server->main_ew))
        goto error;

    atomic_init(&server->running, true);

    if (!server_tm_start_threads(server))
        goto error;   

    if (!server_wait_for_workers(server))
        goto error;

    if (!server_init_eventfd(server->main_ew))
        goto error;

    if (!server_init_signal(server->main_ew))
        goto error;

    // if --fork is used, fork and exit parent process 
    // e.i. becomes a background process
    if (server->conf.fork)
    {
        if (fork() != 0)
        {
            // Parent
            exit(0);
        }
    }

    return server;
error:;
    server_cleanup(server);
    return NULL;
}
