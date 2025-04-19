#include "chat/user_file.h"
#include "chat/db.h"
#include "server.h"

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
