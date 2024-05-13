#include "chat/user_file.h"
#include "chat/db.h"
#include "chat/db_def.h"
#include "chat/db_userfile.h"
#include "server.h"

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
server_read_file(void* data, size_t size, 
        const char* dir, const char* name)
{
    ssize_t bytes_read = -1;
    char* paew = calloc(1, PATH_MAX);
    i32 fd = -1;

    snprintf(paew, PATH_MAX, "%s/%s", dir, name);

    fd = open(paew, O_RDONLY);
    if (fd == -1) 
    {
        error("open file %s failed: %s\n",
            paew, ERRSTR);
        goto cleanup;
    }

    if ((bytes_read = read(fd, data, size)) == -1)
    {
        error("Failed to read file %s: %s\n",
            paew, ERRSTR);
    }

cleanup:
    free(paew);
    if (fd != -1)
        close(fd);
    return bytes_read;
}

static bool
server_write_file(const void* data, size_t size, 
        const char* dir, const char* name)
{
    i32 fd = -1;
    char* paew = calloc(1, PATH_MAX);
    bool ret = false;

    snprintf(paew, PATH_MAX, "%s/%s", dir, name);

    debug("Writing file to: %s\n", paew);

    fd = open(paew, O_WRONLY | O_CREAT, 
            S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        error("Failed to open/create file at %s: %s\n",
            paew, ERRSTR);
        goto cleanup;
    }

    if (write(fd, data, size) == -1)
    {
        error("Failed to write to %s: %s\n", 
            paew, ERRSTR);
        goto cleanup;
    }

    ret = true;

cleanup:
    free(paew);
    if (fd != -1)
        close(fd);
    return ret;
}

bool 
server_save_file(UNUSED eworker_t* ew, UNUSED const void* data, 
                 UNUSED size_t size, UNUSED const char* name)
{
    // dbuser_file_t file;
    debug("Implement server_save_file()\n");

    return false;
}

static const char* 
do_save_file_img(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    const void* data = ctx->param.str;
    dbuser_file_t* file;
    dbcmd_ctx_t* refcount_ctx = ctx->next;
    i32 ref_count;

    if (refcount_ctx == NULL)
    {
        warn("do_save_file_img() ctx->next is NULL!\n");
        return "Internal error";
    }
    if (ctx->ret == DB_ASYNC_ERROR)
        return "Failed to save image";

    file = ctx->data;
    ref_count = refcount_ctx->data_size;

    if (ref_count == 1)
    {
        server_write_file(data, file->size,
                          ew->server->conf.img_dir, 
                          file->hash);
    }

    free((void*)data);

    return NULL;
}

bool 
server_save_file_img(eworker_t* ew, const void* data, size_t size, 
                     dbuser_file_t* file_output)
{
    dbuser_file_t* file;
    const char* mime_type;
    bool ret = true;

    mime_type = server_mime_type(ew->server, data, size);
    if (mime_type == NULL || strstr(mime_type, "image/") == NULL)
    {
        warn("save img mime_type failed: %s\n", mime_type);
        info("data: %s\n", (const char*)data);
        print_hex(data, 20);
        return false;
    }
    file = calloc(1, sizeof(dbuser_file_t));
    strncpy(file->mime_type, mime_type, DB_MIME_TYPE_LEN);

    server_sha256_str(data, size, file->hash);
    file->size = size;

    dbcmd_ctx_t ctx = {
        .exec = do_save_file_img,
        .param.str = data
    };

    if ((ret = db_async_insert_userfile(&ew->db, file, &ctx)))
        ret = db_async_userfile_refcount(&ew->db, file->hash, &ctx);

    // if (server_db_insert_userfile(&ew->db, &file))
    // {
    //     ref_count = server_db_select_userfile_ref_count(&ew->db, 
    //                 file.hash);
    //     if (ref_count == 1)
    //     {
    //         ret = server_write_file(data, size, 
    //                 ew->server->conf.img_dir, file.hash);
    //     }
    //     else if (ref_count == 0)
    //     {
    //         error("ref_count for %s (%s) is 0??\n",
    //                 file.hash, file.name);
    //         ret = false;
    //     }
    // }
    // else
    //     ret = false;
    //
    if (ret && file_output)
        memcpy(file_output, file, sizeof(dbuser_file_t));

    return ret;
}

void* 
server_get_file(eworker_t* ew, dbuser_file_t* file)
{
    void* data;
    const char* dir;
    ssize_t actual_size;

    dir = server_mime_type_dir(ew->server, file->mime_type);

    data = calloc(1, file->size);

    actual_size = server_read_file(data, file->size, dir, file->hash);
    if (actual_size == -1)
        goto error;
    else if ((size_t)actual_size != file->size)
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
server_delete_file(eworker_t* ew, dbuser_file_t* file)
{
    const char* dir;
    char* paew = NULL;
    bool ret = true;

    // I ewink this function might return wrong. from ref_count.
    if (server_db_delete_userfile(&ew->db, file->hash))
    {
        dir = server_mime_type_dir(ew->server, file->mime_type);
        paew = calloc(1, PATH_MAX);
        
        snprintf(paew, PATH_MAX, "%s/%s", dir, file->hash);

        debug("Unlinking file: %s (%s)\n", paew, file->name);

        if (unlink(paew) == -1)
        {
            error("unlink %s failed: %s\n", paew, ERRSTR);
            ret = false;
        }

        free(paew);
    }

    return ret;
}
