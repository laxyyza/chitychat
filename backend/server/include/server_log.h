#ifndef _SERVER_LOG_H_
#define _SERVER_LOG_H_

enum server_log_level
{
    SERVER_FATAL,
    SERVER_ERROR,
    SERVER_WARN,
    SERVER_INFO,
    SERVER_DEBUG,
    SERVER_VERBOSE,

    SERVER_LOG_LEVEL_LEN
};

void server_set_loglevel(enum server_log_level);
enum server_log_level server_get_loglevel(void);
void server_log(enum server_log_level level, const char* filename, int line, const char* format, ...);

#define fatal(format, ...)  server_log(SERVER_FATAL,    __FILE_NAME__,  __LINE__,   format,     ##__VA_ARGS__)
#define error(format, ...)  server_log(SERVER_ERROR,    __FILE_NAME__,  __LINE__,   format,     ##__VA_ARGS__)
#define warn(format, ...)   server_log(SERVER_WARN,     __FILE_NAME__,  __LINE__,   format,     ##__VA_ARGS__)
#define info(format, ...)   server_log(SERVER_INFO,     NULL,           0,          format,     ##__VA_ARGS__)
#define debug(format, ...)  server_log(SERVER_DEBUG,    NULL,           0,          format,     ##__VA_ARGS__)
#define verbose(format, ...)  server_log(SERVER_VERBOSE,    NULL,           0,          format,     ##__VA_ARGS__)

#endif // _SERVER_LOG_H_