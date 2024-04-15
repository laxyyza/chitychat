#ifndef _COMMON_H_
#define _COMMON_H_  

#define _GNU_SOURCE

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/random.h>
#include <sys/timerfd.h>
#include <fcntl.h>
#include <libgen.h>
#include <getopt.h>
#include <json-c/json.h>
#include <linux/limits.h>
#include <magic.h>
#include <json-c/json.h>

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;
typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;

typedef struct server   server_t;
typedef struct http     http_t;

#define ERRSTR strerror(errno)
#define UNUSED __attribute__((unused))

#define SYSTEM_USERNAME_LEN 64

#endif //_COMMON_H_
