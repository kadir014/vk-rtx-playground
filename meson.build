project(
    'vk-rtx-playground',
    ['c',],
    license: 'MIT',
    license_files: 'LICENSE',
    default_options: [
        'c_std=c99',
        'warning_level=1',
        'default_library=static'
    ]
)


compiler = meson.get_compiler('c')

if compiler.get_id() == 'msvc'
    c_args = ['/arch:AVX2', '/D_CRT_SECURE_NO_WARNINGS']
    link_args = []

    # c_args += ['/wd4305']
else
    # -march=mavx2 ?
    c_args = []
    link_args = ['-lm']

    # When you target C99, you also have to specify POSIX clock
    # https://raspberrypi.stackexchange.com/a/95480
    c_args += '-D_POSIX_C_SOURCE=200809L'
endif


python = find_program('python3', 'python')
r = run_command(python, 'scripts/install_wraps.py')
if r.returncode() != 0
    error('install_wraps.py script failed.\n', r.stderr())
endif

main_src = [
    'src/main.c',
]
main_includes = ['include']

sdl2_dep = dependency(
    'sdl2',
    default_options: {'use_video_vulkan': 'enabled'}
)

if compiler.get_id() == 'msvc'
    sdl2_main_dep = dependency('sdl2main')
    main_deps = [sdl2_dep, sdl2_main_dep,]
else
    main_deps = [sdl2_dep,]
endif

executable(
    'main',
    sources: main_src,
    include_directories: main_includes,
    c_args: c_args,
    link_args: link_args,
    dependencies: main_deps,
)