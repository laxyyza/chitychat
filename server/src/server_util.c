#include "server_util.h"
#include "server_log.h"

#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>

char* 
strsplit(char* restrict str, const char* restrict delm, char** restrict saveprr)
{
    char* start;
    char* ret;
    char** buffer = saveprr;
    const size_t delm_len = strlen(delm);

    if (str)
    {
        buffer = saveprr;
        *buffer = str;
    }
    if (!str && (!buffer || !*buffer))
        return NULL;

    start = strstr(*buffer, delm);
    if (start == NULL)
    {
        ret = *(*buffer) ? *buffer : NULL;
        buffer = NULL;
        return ret;
    }

    for (size_t i = 0; i < delm_len; i++)
        start[i] = 0x00;

    ret = *buffer;
    (*buffer) = start + delm_len;

    return ret;
}

static void 
print_hex_char(const char* str, size_t max, size_t start, size_t per_line)
{
    size_t line_i = 0;
    for (size_t c = start; c < max; c++)
    {
        const char ch = str[c];
        if (ch >= 32 && ch <= 126)
            putchar(ch);
        else
            putchar('.');

        line_i++;

        if (line_i >= per_line)
        {
            line_i = 0;
            break;
        }
    }
}

void 
print_hex(const char* str, size_t len)
{
    size_t per_line = 20;
    size_t line_i = 0;
    size_t ci = 0;

    for (size_t i = 0; i < len; i++)
    {
        const u8 ch = str[i];
        printf("%02X ", ch);
        line_i++;
        if (line_i >= per_line)
        {
            printf("\t");
            print_hex_char(str, len, ci, per_line);
            line_i = 0;
            ci = i;
            putchar('\n');
        }
    }
    for (size_t i = 0; i < per_line - line_i; i++)
    {
        printf("   ");
    }
    putchar('\t');

    print_hex_char(str, len, ci, per_line);

    printf("\n");
}

size_t 
fdsize(i32 fd)
{
    struct stat stat;
    if (fstat(fd, &stat) == -1)
        error("fstat on fd: %s\n", fd, ERRSTR);
    return stat.st_size;
}

void 
swpcpy(u8* restrict dest, const u8* restrict src, size_t n) 
{
    for (size_t i = 0; i < n; i++) 
        dest[i] = src[n - 1 - i];
}

void 
mask(u8* buf, size_t buf_len, const u8* maskkey, size_t maskkey_len)
{
    for (size_t i = 0; i < buf_len; i++)
        buf[i] ^= maskkey[i % maskkey_len];
}

i32 
file_isdir(const char* filepath)
{
    if (!filepath)
        return -1;

    struct stat file_stat;
    if (stat(filepath, &file_stat) == -1)
    {
        error("stat on '%s': %s\n", filepath, ERRSTR);
        return -1;
    }

    return S_ISDIR(file_stat.st_mode);
}

void* 
combine_buffers(struct iovec* iov, size_t n, size_t* size_ptr)
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

void hexstr_to_u8(const char* hexstr, size_t hexstr_len, u8* output)
{
    if (hexstr[0] == '\\' && hexstr[1] == 'x')
    {
        hexstr += 2;
        hexstr_len -= 2;
    }
    size_t index = 0;

    for (size_t i = 0; i < hexstr_len; i += 2)
    {
        char hexpair[3];
        hexpair[0] = hexstr[i];
        hexpair[1] = hexstr[i + 1];
        hexpair[2] = 0x00;
        char* endptr;

        output[index] = (u8)strtoul(hexpair, &endptr, 16);
        index++;
    }
}