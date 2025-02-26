project('chitychat', 'c', 
    default_options: [
        'c_std=c23',
        'werror=true',
        'warning_level=2',
    ],
    version: run_command('cat', join_paths(meson.project_source_root(), 'VERSION'), check: true).stdout().strip(),
)

add_global_arguments(
    '-Wno-deprecated-declarations', 
    language: 'c',
)

subdir('server/lib/cutils')

openssl_dep = dependency('openssl')
jsonc_dep = dependency('json-c')
libpq_dep = dependency('libpq')
magic_dep = dependency('libmagic')

server_src = files(
    'server/src/main.c',
    'server/src/server.c',
    'server/src/server_log.c',
    'server/src/server_client.c',
    'server/src/server_http.c',
    'server/src/server_websocket.c',
    'server/src/server_util.c',
    'server/src/server_crypt.c',
    'server/src/server_init.c',
    'server/src/server_events.c',
    'server/src/server_timer.c',
    'server/src/server_http_get.c',
    'server/src/server_http_post.c',
    'server/src/server_tm.c',
    'server/src/server_ht.c',
    'server/src/server_signal.c',
    'server/src/server_eworker.c',

    'server/src/chat/user_file.c',
    'server/src/chat/user_login.c',
    'server/src/chat/group.c',
    'server/src/chat/user.c',
    'server/src/chat/ws_text_frame.c',
    #'server/src/chat/user_session.c',
    'server/src/chat/upload_token.c',
    'server/src/chat/db.c',
    'server/src/chat/db_user.c',
    'server/src/chat/db_group.c',
    'server/src/chat/db_pipeline.c',
    'server/src/chat/db_userfile.c',
    'server/src/chat/user_upload.c',
    'server/src/chat/rtusm.c',
    'server/src/chat/cmd.c',
    'server/src/chat/db_user_session.c',
)
include_dirs = [include_directories('server/include')]
include_dirs += cutils_include

executable('chitychat', server_src, 
    include_directories: include_dirs,
    dependencies: [
        openssl_dep, 
        jsonc_dep, 
        libpq_dep, 
        magic_dep
    ],
    link_with: libcutils
)
