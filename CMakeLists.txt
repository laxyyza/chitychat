cmake_minimum_required(VERSION 3.27.0)
project("Chity Chat" LANGUAGES C)

add_executable(chitychat)
target_sources(chitychat PRIVATE
    server/src/main.c
    server/src/server.c
    server/src/server_log.c
)

target_include_directories(chitychat PRIVATE
    server/include
)

target_link_libraries(chitychat PRIVATE 
    ssl
    crypto
    json-c
)