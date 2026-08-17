# vk-rtx-playground
My playground and toy framework trying to learn Vulkan and hardware raytracing!
<br>
<img width=600 src="https://raw.githubusercontent.com/kadir014/vk-rtx-playground/refs/heads/main/assets/thumb.png">



# Installation & Running
### 📌 Prerequisites
- Compiler that supports C99 and C++17 (VMA needs C++17 or newer for `aligned_alloc`)
- [Python 3.12+](https://www.python.org/downloads/)
- [Meson build system](https://mesonbuild.com/Getting-meson.html)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

**1.** First, clone the repository.
```shell
$ git clone https://github.com/kadir014/vk-rtx-playground.git
$ cd vk-rtx-playground
```

**2.** Before building the project, start by compiling the shaders. This will also let you know if you have the Vulkan SDK properly installed.
```shell
$ python compile_shaders.py
```

**3.** Then setup the meson environment. If you want to build in release mode, you can use `--buildtype=release` ([See meson's build type options](https://mesonbuild.com/Builtin-options.html#details-for-buildtype)).
```shell
$ meson setup build_dir --buildtype=debug --wipe
```

> [!IMPORTANT]
> On Windows, you may need to run `meson setup` twice for SDL2 to install properly.

**4.** Compile. If everything goes correctly, you should be able to run the binary.
```shell
$ cd build_dir
$ meson compile
$ ./main
```

> [!NOTE]
> On Linux, if you can't move around in the application, your display protocol might be messing with key inpus. Try forcing either `wayland` or `x11` while launching the binary:
> ```shell
> $ SDL_VIDEODRIVER=x11 ./main
> ```


# Resources & References
- [Vulkan Tutorial, by Alexander Overvoorde](https://vulkan-tutorial.com/)
- [Vulkan Documentation — Khronos Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/00_Introduction.html)
- [Vulkan Specification](https://docs.vulkan.org/spec/latest/index.html)
- [Vulkan API Reference](https://docs.vulkan.org/refpages/latest/refpages/index.html)
- [AMD GPUOpen — Understanding Vulkan® Objects](https://gpuopen.com/learn/understanding-vulkan-objects/)



# Attributions
- [Stanford Bunny](https://graphics.stanford.edu/data/3Dscanrep/) — Stanford Computer Graphics Laboratory
- ["Fantasy Table Low Poly Stylised" by Kadre](https://sketchfab.com/3d-models/fantasy-table-low-poly-stylised-50a4b86e49ce4b52a7068c79e7f04c06) — Licensed under [CC-BY](https://creativecommons.org/licenses/by/4.0/)
- ["Derelict Airfield 02" HDRI sky texture](https://polyhaven.com/a/derelict_airfield_02) — Licensed under [CC0](https://polyhaven.com/license)
- ["Statue photograph" by Rodrigo Curi](https://unsplash.com/photos/brown-concrete-statue-under-blue-sky-during-daytime-TqSgXhalVQ0) — Licensed under [Unsplash License](https://unsplash.com/license)



# License
[MIT](LICENSE) © Kadir Aksoy