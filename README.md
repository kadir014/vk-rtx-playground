# vk-rtx-playground
My playground and toy framework trying to learn Vulkan and hopefully hardware raytracing...



# Installation & Running
### 📜 Prerequisites
- C compiler that supports C99
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
> On Windows, you may need to run the above command twice for SDL2 to install properly.

**3.** Compile. If everything goes correctly, you should be able to run the binary.
```shell
$ cd build_dir
$ meson compile
$ ./main
```


# Resources
- [vulkan-tutorial.com](https://vulkan-tutorial.com/)
- [Khronos Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/00_Introduction.html)


# License
[MIT](LICENSE) © Kadir Aksoy