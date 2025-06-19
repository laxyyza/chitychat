#ifndef _SERVER_UTIL_H_
#define _SERVER_UTIL_H_

#include "common.h"

bool        str_startwith(const char* restrict str, const char* restrict start);
char*       strsplit(char* restrict str, const char* restrict delm, char** restrict saveptr);
void        print_hex(const char* str, size_t len);
size_t      fdsize(i32 fd);
i32         file_isdir(const char* filepath);
void        mask(u8* buf, size_t buf_len, const u8* maskkey, size_t maskkey_len);
void*       combine_buffers(struct iovec* iov, size_t n, size_t* size_ptr);
void        hexstr_to_u8(const char* hexstr, size_t hexstr_len, u8* output);
void        bytes_to_hex(const u8* buf, u32 buf_size, char* out_hex);
const char* server_get_content_type(const char* path);
const char* getenvd(const char* var, const char* default_val);
void        uuid_to_u64_2(const char* uuid_str, u64 out[2]);
u32         strncpy_replace(char* dst, const char* src, u32 n, char old, char new);
void        strncpy_tolower(char* dst, const char* src, u64 n);
i32         strncmp_s1lower(const char* s1, const char* s2, u64 n);

// Copy n bytes swapped order
void        swpcpy(u8* restrict dest, const u8* restrict src, size_t n);

#define TIME_FUNC_NS(func)\
    ({\
        struct timespec __start;\
        struct timespec __end;\
        clock_gettime(CLOCK_MONOTONIC, &__start);\
        __typeof__(func) __ret = func;\
        clock_gettime(CLOCK_MONOTONIC, &__end);\
        long long int __time_taken = (__end.tv_sec - __start.tv_sec) * 1000000000LL +\
                               (__end.tv_nsec - __start.tv_nsec);\
        info("%s: took %lldns.\n", #func, __time_taken);\
        __ret;\
    })

#endif // _SERVER_UTIL_H_
