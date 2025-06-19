project('chitychat', 'c', 
    default_options: [
        'c_std=c23',
        'werror=true',
        'warning_level=2',
        'default_library=static',
    ],
    subproject_dir: 'backend/server/lib/',
    version: run_command('cat', join_paths(meson.project_source_root(), 'VERSION'), check: true).stdout().strip(),
)

subdir('backend/server')