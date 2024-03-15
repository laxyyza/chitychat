#ifndef _SERVER_UTIL_H_
#define _SERVER_UTIL_H_

#include "common.h"
#include <sys/uio.h>

char* strsplit(char* restrict str, const char* restrict delm, char** restrict saveptr);
void print_hex(const char* str, size_t len);
size_t fdsize(int fd);
i32     file_isdir(const char* filepath);

// Copy n bytes swapped order
void swpcpy(u8* restrict dest, const u8* restrict src, size_t n);
void mask(u8* buf, size_t buf_len, const u8* maskkey, size_t maskkey_len);
void* combine_buffers(struct iovec* iov, size_t n, size_t* size_ptr);

#endif // _SERVER_UTIL_H_