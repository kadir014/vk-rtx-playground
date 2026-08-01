# vk-rtx-playground
My playground and toy framework trying to learn Vulkan and hopefully hardware raytracing...



# Installation & Running
### 📜 Prerequisites
- Compiler that supports C99 & C++11
- [Meson build system](https://mesonbuild.com/Getting-meson.html)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

**1.** First, clone the repository.
```shell
$ git clone https://github.com/kadir014/vk-rtx-playground.git
$ cd vk-rtx-playground
```

**2.** Then setup the meson environment. If you want to build in release mode, you can use `--buildtype=release` ([See meson's build type options](https://mesonbuild.com/Builtin-options.html#details-for-buildtype)).
```shell
$ meson setup build_dir --buildtype=debug --wipe
```

> [!IMPORTANT]
> On Windows, you may need to run `meson setup` twice for SDL2 to install properly.

**3.** Compile. If everything goes correctly, you should be able to run the binary.
```shell
$ cd build_dir
$ meson compile
$ ./main
```


# Resources
- [Vulkan Tutorial, by Alexander Overvoorde](https://vulkan-tutorial.com/)
- [Vulkan Documentation — Khronos Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/00_Introduction.html)
- [AMD GPUOpen — Understanding Vulkan® Objects](https://gpuopen.com/learn/understanding-vulkan-objects/)



# Attributions
- [Stanford Bunny](https://graphics.stanford.edu/data/3Dscanrep/) — Stanford Computer Graphics Laboratory
- [Low Poly Stanford Bunny](https://www.thingiverse.com/thing:151081) — Licensed under [CC-BY-NC](https://creativecommons.org/licenses/by-nc/4.0/)
- ["Derelict Airfield 02" HDRI sky texture](https://polyhaven.com/a/derelict_airfield_02) — Licensed under [CC0](https://polyhaven.com/license)
- [Statue photograph by Rodrigo Curi](https://unsplash.com/photos/brown-concrete-statue-under-blue-sky-during-daytime-TqSgXhalVQ0) — Licensed under [Unsplash License](https://unsplash.com/license)



# License
[MIT](LICENSE) © Kadir Aksoy