#include "server_util.h"
#include "server_log.h"

#include <sys/stat.h>

char* strsplit(char* restrict str, const char* restrict delm, char** restrict saveprr)
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

static void print_hex_char(const char* str, size_t max, size_t start, size_t per_line)
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

void print_hex(const char* str, size_t len)
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

size_t fdsize(i32 fd)
{
    struct stat stat;
    if (fstat(fd, &stat) == -1)
        error("fstat on fd: %s\n", fd, ERRSTR);
    return stat.st_size;
}

void swpcpy(u8* restrict dest, const u8* restrict src, size_t n) 
{
    for (size_t i = 0; i < n; i++) 
        dest[i] = src[n - 1 - i];
}

void mask(u8* buf, size_t buf_len, const u8* maskkey, size_t maskkey_len)
{
    for (size_t i = 0; i < buf_len; i++)
        buf[i] ^= maskkey[i % 4];
}