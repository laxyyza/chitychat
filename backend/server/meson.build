add_global_arguments(
    '-Wno-deprecated-declarations', 
    language: 'c',
)

subdir('lib/cutils')

openssl_dep = dependency('openssl')
jsonc_dep = dependency('json-c')
libpq_dep = dependency('libpq')
magic_dep = dependency('libmagic')

server_src = files(
    'src/main.c',
    'src/server.c',
    'src/server_log.c',
    'src/server_client.c',
    'src/server_http.c',
    'src/server_websocket.c',
    'src/server_util.c',
    'src/server_crypt.c',
    'src/server_init.c',
    'src/server_events.c',
    'src/server_timer.c',
    'src/server_http_get.c',
    'src/server_http_post.c',
    'src/server_tm.c',
    'src/server_ht.c',
    'src/server_signal.c',
    'src/server_eworker.c',
    'src/backend.c',

    'src/chat/user_file.c',
    'src/chat/user_login.c',
    'src/chat/group.c',
    'src/chat/user.c',
    'src/chat/ws_text_frame.c',
    #'src/chat/user_session.c',
    'src/chat/upload_token.c',
    'src/chat/db.c',
    'src/chat/db_user.c',
    'src/chat/db_group.c',
    'src/chat/db_pipeline.c',
    'src/chat/db_userfile.c',
    'src/chat/user_upload.c',
    'src/chat/rtusm.c',
    'src/chat/cmd.c',
    'src/chat/db_user_session.c',
)
include_dirs = [include_directories('include')]
include_dirs += cutils_include

executable('cc_server', server_src, 
    include_directories: include_dirs,
    dependencies: [
        openssl_dep, 
        jsonc_dep, 
        libpq_dep, 
        magic_dep
    ],
    link_with: libcutils
)