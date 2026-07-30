project(
    'vk-rtx-playground',
    ['c', 'cpp'],
    license: 'MIT',
    license_files: 'LICENSE',
    default_options: [
        'c_std=c99',
        'cpp_std=c++11',
        'warning_level=1',
        'default_library=static'
    ],
    meson_version: '>=1.2.0', # For dependency default options
)


compiler = meson.get_compiler('c')

if compiler.get_id() == 'msvc'
    c_args = ['/arch:AVX2', '/D_CRT_SECURE_NO_WARNINGS']
    link_args = []
else
    # -march=mavx2 ?
    c_args = []
    link_args = ['-lm']

    # When you target C99, you also have to specify POSIX clock
    # https://raspberrypi.stackexchange.com/a/95480
    c_args += '-D_POSIX_C_SOURCE=200809L'
endif


lava_src = [
    'src/lava/core/clock.c',
    'src/lava/core/io.c',

    'src/lava/containers/array.c',
    'src/lava/containers/refarray.c',

    'src/lava/vk/context.c',
    'src/lava/vk/swapchain.c',
    'src/lava/vk/buffer.c',
    'src/lava/vk/graphics_pipeline.c',

    'src/vma_impl.cpp',
]

lava_includes = ['include']


python = find_program('python3', 'python')
r = run_command(python, 'scripts/install_wraps.py')
if r.returncode() != 0
    error('install_wraps.py script failed.\n', r.stderr())
endif

main_src = ['src/main.c',] + lava_src

sdl2_dep = dependency(
    'sdl2',
    default_options: {'use_video_vulkan': 'enabled'}
)

vulkan_dep = dependency('vulkan')
vma_dep = dependency('VulkanMemoryAllocator')
cglm_dep = dependency('cglm')

if compiler.get_id() == 'msvc'
    sdl2_main_dep = dependency('sdl2main')
    main_deps = [sdl2_dep, sdl2_main_dep, vulkan_dep, vma_dep, cglm_dep]
else
    main_deps = [sdl2_dep, vulkan_dep, vma_dep, cglm_dep]
endif

executable(
    'main',
    sources: main_src,
    include_directories: lava_includes,
    c_args: c_args,
    cpp_args: [],
    link_args: link_args,
    dependencies: main_deps,
)


# Unit tests

tests_src = ['tests/main.c'] + lava_src

executable(
    'tests',
    sources: tests_src,
    include_directories: lava_includes + ['tests'],
    c_args: c_args,
    link_args: link_args,
    dependencies: main_deps,
)