
# Lum-al
**Lum-al** is abstraction layer on top of Vulkan, originally designed as a part of [**Lum engine**](https://github.com/platonvin/lum), but extracted to be used in other projects

this is very thin C++ Vulkan abstraction, built for usecase when objects (renderpasses, pipelines, framebuffers, descriptors) are mostly defined at initializon time

to prevent your language server from parsing files it should not, define _LANG_SERVER (for example, with -D_LANG_SERVER argument) for it

[examples/triangle.cpp](examples/triangle.cpp) contains simple example to draw triangle with 2 subpasses - main shading (writes to intermediate image) and posteffect (reads from intermediate image via supass input and then writes to swapchain)

[Mangaka](https://github.com/platonvin/mangaka) contains more complicated example - lightmapping, depth prepass and cel shading (in manga style)

## Installation 
 
- ### Prerequisites 
 
  - **C++ Compiler** : [MSYS2 MinGW](https://www.msys2.org/)  recommended for Windows. For Linux, prefer GNU C++.
 
  - **Vcpkg** : follow instructions at [Vcpkg](https://vcpkg.io/en/getting-started) .
 
  - **Make** : for Windows, install with MinGW64. For Linux, typically installed by default.
 
  - **Vulkan SDK** : ensure that you have the [Vulkan SDK](https://vulkan.lunarg.com/) if you want debug features (**on** by default)
 
- ### Steps 

  - Ensure you have a C++20 compiler, Vcpkg, and Make.
 
  - Clone the repository
    `$ git clone https://github.com/platonvin/lum-al.git` 
 
  - Navigate to the project directory:
`$ cd lum-al`
 
  - Install dependencies via Vcpkg:
`$ vcpkg install` 
    - On Linux, GLFW may ask you to install multiple packages. Install them in advance:
`sudo apt install libxinerama-dev libxcursor-dev xorg-dev libglu1-mesa-dev pkg-config build-essential`.
 
    - For MinGW on Windows specify MinGW as triplet:
`$ vcpkg install --triplet=x64-mingw-static --host-triplet=x64-mingw-static`.
 
  - Build library:
`$ make library -j4`
     - Built library is placed into /lib. Use -llumal to link with it (do not forget to -Llum-al/lib)
 - use `make example` to build and run example-triangle.

see [src/al.hpp](src/al.hpp) for more info

Some Vulkan function are wrapped with Renderer:: ones, but only when it simplifies usage