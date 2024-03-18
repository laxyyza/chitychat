#include "server_file.h"
#include <magic.h>
#include "server.h"
#include <fcntl.h>

static const char*
server_mime_type_dir(server_t* server, const char* mime_type)
{
    const char* ret;

    if (strstr(mime_type, "image/"))
        ret = server->conf.img_dir;
    else if (strstr(mime_type, "video/"))
        ret = server->conf.vid_dir;
    else
        ret = server->conf.file_dir;

    return ret;
}

bool 
server_init_magic(server_t* server)
{
    server->magic_cookie = magic_open(MAGIC_MIME_TYPE);
    if (server->magic_cookie == NULL)
    {
        error("Unable to initialize magic library\n");
        return false;
    }
 
    if (magic_load(server->magic_cookie, NULL) != 0)
    {
        error("Unable to load magic database\n");
        return false;
    }

    return true;
}

void 
server_close_magic(server_t* server)
{
    magic_close(server->magic_cookie);
}

const char* 
server_mime_type(server_t* server, const void* data, size_t size)
{
    const char* mime_type = magic_buffer(server->magic_cookie, data, size);
    return mime_type;
}

static ssize_t
server_read_file(server_t* server, void* data, size_t size, 
        const char* dir, const char* name)
{
    ssize_t bytes_read = -1;
    char* path = calloc(1, PATH_MAX);
    i32 fd = -1;

    snprintf(path, PATH_MAX, "%s/%s", dir, name);

    fd = open(path, O_RDONLY);
    if (fd == -1) 
    {
        error("open file %s failed: %s\n",
            path, ERRSTR);
        goto cleanup;
    }

    if ((bytes_read = read(fd, data, size)) == -1)
    {
        error("Failed to read file %s: %s\n",
            path, ERRSTR);
    }

cleanup:
    free(path);
    if (fd != -1)
        close(fd);
    return bytes_read;
}

static bool
server_write_file(server_t* server, const void* data, size_t size, 
        const char* dir, const char* name)
{
    i32 fd = -1;
    char* path = calloc(1, PATH_MAX);
    bool ret = false;

    snprintf(path, PATH_MAX, "%s/%s", dir, name);

    fd = open(path, O_WRONLY | O_CREAT, 
            S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        error("Failed to open/create file at %s: %s\n",
            path, ERRSTR);
        goto cleanup;
    }

    if (write(fd, data, size) == -1)
    {
        error("Failed to write to %s: %s\n", 
            path, ERRSTR);
        goto cleanup;
    }

    ret = true;

cleanup:
    free(path);
    if (fd != -1)
        close(fd);
    return ret;
}

bool 
server_save_file(server_t* server, const void* data, size_t size, const char* name)
{
    dbuser_file_t file;

    return false;
}

bool 
server_save_file_img(server_t* server, const void* data, size_t size, const char* name, dbuser_file_t* file_output)
{
    dbuser_file_t file;
    const char* mime_type;
    i32 ref_count;
    bool ret = true;

    memset(&file, 0, sizeof(dbuser_file_t));

    mime_type = server_mime_type(server, data, size);
    if (mime_type == NULL || strstr(mime_type, "image/") == NULL)
    {
        warn("save img mime_type failed: %s\n", mime_type);
        info("data: %s\n", (const char*)data);
        print_hex(data, 20);
        return false;
    }
    strncpy(file.mime_type, mime_type, DB_MIME_TYPE_LEN);

    server_sha256_str(data, size, file.hash);
    file.size = size;

    strncpy(file.name, name, DB_PFP_NAME_MAX);

    if (server_db_insert_userfile(server, &file))
    {
        ref_count = server_db_select_userfile_ref_count(server, 
                    file.hash);
        if (ref_count == 1)
        {
            ret = server_write_file(server, data, size, 
                    server->conf.img_dir, file.hash);
        }
        else if (ref_count == 0)
        {
            error("ref_count for %s (%s) is 0??\n",
                    file.hash, file.name);
            ret = false;
        }
    }
    else
        ret = false;

    if (ret && file_output)
        memcpy(file_output, &file, sizeof(dbuser_file_t));

    return ret;
}

void* 
server_get_file(server_t* server, dbuser_file_t* file)
{
    void* data;
    const char* dir;
    size_t actual_size;
    i32 fd;

    dir = server_mime_type_dir(server, file->mime_type);

    data = calloc(1, file->size);

    actual_size = server_read_file(server, data, file->size, dir, file->hash);
    if (actual_size == -1)
        goto error;
    else if (actual_size != file->size)
    {
        warn("acti_size != file->size: %zu != %zu\n",
                actual_size, file->size);
        file->size = actual_size;
    }
    
    return data;
error:
    free(data);
    return NULL;
}

bool 
server_delete_file(server_t* server, dbuser_file_t* file)
{
    const char* dir;
    char* path = NULL;
    bool ret = true;

    // I think this function might return wrong. from ref_count.
    if (server_db_delete_userfile(server, file->hash))
    {
        dir = server_mime_type_dir(server, file->mime_type);
        path = calloc(1, PATH_MAX);
        
        snprintf(path, PATH_MAX, "%s/%s", dir, file->hash);

        debug("Unlinking file: %s (%s)\n", path, file->name);

        if (unlink(path) == -1)
        {
            error("unlink %s failed: %s\n", path, ERRSTR);
            ret = false;
        }

        free(path);
    }

    return ret;
}