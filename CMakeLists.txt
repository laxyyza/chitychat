cmake_minimum_required(VERSION 3.27.0)
project("Chity Chat" LANGUAGES C)

set(CMAKE_C_FLAGS "-Werror -Wno-deprecated-declarations")

add_executable(chitychat)
target_sources(chitychat PRIVATE
    server/src/main.c
    server/src/server.c
    server/src/server_log.c
    server/src/server_client.c
    server/src/server_http.c
    server/src/server_websocket.c
    server/src/server_db.c
    server/src/server_util.c
)

target_include_directories(chitychat PRIVATE
    server/include
)

target_link_libraries(chitychat PRIVATE 
    ssl
    crypto
    json-c
    sqlite3
)