#include "server_util.h"
#include "server_log.h"

#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <ctype.h>

bool 
str_startwith(const char* restrict str, 
              const char* restrict start)
{
    size_t start_len;

    if (!str || !start)
        return false;

    start_len = strlen(start);

    return strncmp(str, start, start_len) == 0;
}

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
        if (errno == ENOENT)
            debug("stat on '%s': %s\n", filepath, ERRSTR);
        else
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

void 
hexstr_to_u8(const char* hexstr, size_t hexstr_len, u8* output)
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

void
bytes_to_hex(const u8* buf, u32 buf_size, char* out_hex)
{
    static const char hex_chars[] = "0123456789ABCDEF";

    for (u32 i = 0; i < buf_size; i++)
    {
        out_hex[2 * i]      = hex_chars[(buf[i] >> 4) & 0x0F];
        out_hex[2 * i + 1]  = hex_chars[buf[i] & 0x0F];
    }
    out_hex[buf_size * 2] = 0x00; // null-terminate string.
}

const char* 
server_get_content_type(const char* path)
{
#define CMP_EXT(ext) !strcmp(extention, ext)

    const char* extention = strrchr(path, '.');
    if (extention)
    {
        extention++;
        if (CMP_EXT("html"))
            return "text/html";
        else if (CMP_EXT("css"))
            return "text/css";
        else if (CMP_EXT("csv"))
            return "text/csv";
        else if (CMP_EXT("js"))
            return "application/javascript";
        else if (CMP_EXT("png"))
            return "image/png";
        else if (CMP_EXT("gif"))
            return "image/gif";
        else if (CMP_EXT("jpg") || CMP_EXT("jpeg"))
            return "image/jpeg";
        else if (CMP_EXT("bmp"))
            return "image/bmp";
        else if (CMP_EXT("svg"))
            return "image/svg+xml";
        else if (CMP_EXT("avif"))
            return "image/avif";
        else if (CMP_EXT("mp4"))
            return "video/mp4";
        else if (CMP_EXT("mov"))
            return "video/quicktime";
        else if (CMP_EXT("ico"))
            return "image/x-icon";
        else if (CMP_EXT("xml"))
            return "application/xml";
        else if (CMP_EXT("zip"))
            return "application/zip";
        else if (CMP_EXT("gz"))
            return "application/gzip";
        else if (CMP_EXT("json"))
            return "application/json";
    }

    return "application/octet-stream";
}

const char*
getenvd(const char* var, const char* default_val)
{
    const char* ret = getenv(var);
    return (ret) ? ret : default_val;
}

void 
uuid_to_u64_2(const char* uuid_str, u64 out[2])
{
    u8 bytes[16];
    sscanf(uuid_str, "%8" SCNx32 "-%4" SCNx16 "-%4" SCNx16 "-%4" SCNx16 "-%12" SCNx64,
           (u32 *)&bytes[0], (u16 *)&bytes[4], (u16 *)&bytes[6],
           (u16 *)&bytes[8], (u64 *)&bytes[10]);

    out[0] = ((u64)bytes[0] << 56) | ((u64)bytes[1] << 48) |
             ((u64)bytes[2] << 40) | ((u64)bytes[3] << 32) |
             ((u64)bytes[4] << 24) | ((u64)bytes[5] << 16) |
             ((u64)bytes[6] << 8)  | ((u64)bytes[7]);

    out[1] = ((u64)bytes[8] << 56) | ((u64)bytes[9] << 48) |
             ((u64)bytes[10] << 40) | ((u64)bytes[11] << 32) |
             ((u64)bytes[12] << 24) | ((u64)bytes[13] << 16) |
             ((u64)bytes[14] << 8)  | ((u64)bytes[15]);
}

u32
strncpy_replace(char* dst, const char* src, u32 n, char old, char new)
{
    u32 i = 0;

    while (src[i])
    {
        char c = src[i];
        if (c == old)
            c = new;
        dst[i] = c;
        i++;

        if (i >= n)
            break;
    }
    dst[i] = 0x00;

    return i;
}

void 
strncpy_tolower(char* dst, const char* src, u64 n)
{
    for (u64 i = 0; i < n && src[i] != 0x00; i++)
        dst[i] = tolower(src[i]);
}

i32
strncmp_s1lower(const char* s1, const char* s2, u64 n)
{
    u64 i = 0;

    while (i < n)
    {
        const u8 c1 = tolower(s1[i]);
        const u8 c2 = s2[i];

        if (c1 != c2) 
            return c1 - c2;

        if (c1 == 0x00)
            return 0;

        i++;
    }

    return 0;
}
