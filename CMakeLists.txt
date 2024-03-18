cmake_minimum_required(VERSION 3.25.0)
project("Chity Chat" LANGUAGES C)

set(CMAKE_C_FLAGS "-Werror -Wall -Wextra -Wshadow -Wstrict-prototypes -Wstrict-overflow=5 -Wunreachable-code -Winit-self -Wuninitialized -Wno-deprecated-declarations")
set(CMAKE_C_STANDARD 23)

add_executable(chitychat)
target_sources(chitychat PRIVATE
    server/src/main.c
    server/src/server.c
    server/src/server_log.c
    server/src/server_client.c
    server/src/server_http.c
    server/src/server_websocket.c
    server/src/server_ws_pld_hdlr.c
    server/src/server_db.c
    server/src/server_util.c
    server/src/server_crypt.c
    server/src/server_file.c
    server/src/server_user_login.c
    server/src/server_user_req.c
    server/src/server_group.c
    server/src/server_user_account.c
)

target_include_directories(chitychat PRIVATE
    server/include
)

target_link_libraries(chitychat PRIVATE 
    ssl
    crypto
    json-c
    sqlite3
    magic
)