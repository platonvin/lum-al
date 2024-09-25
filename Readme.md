[![Build library](https://github.com/platonvin/lum-al/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/platonvin/lum-al/actions/workflows/c-cpp.yml)

# Lum-al
**Lum-al** is abstraction layer on top of Vulkan i designed to simplify development of future projects

this is very thin C++ Vulkan abstraction, built for usecase when objects (renderpasses, pipelines, framebuffers, descriptors) are mostly defined at initializon time

to prevent your language server from parsing files it should not, define _LANG_SERVER (for example, with -D_LANG_SERVER argument) for it

<big>[examples/triangle.cpp](examples/triangle.cpp)</big> contains simple example to draw triangle with 2 subpasses - main shading (writes to intermediate image) and posteffect (reads from intermediate image via supass input and then writes to swapchain)

<big>[Mangaka](https://github.com/platonvin/mangaka)</big> contains more complicated example - lightmapping, depth prepass and cel shading (in manga style)

<big>[Lum](https://github.com/platonvin/lum)</big> is by far the most complicated project built with Lum-al. See repo for more info. Lum-al was originally designed for (_as part of_) Lum 
 
## Installation 
 
- ### Prerequisites 
 
  - **C++ Compiler** : [MSYS2 MinGW](https://www.msys2.org/) recommended for Windows. For Linux, prefer GNU C++.
 
  - **Vcpkg** : follow instructions at [Vcpkg](https://vcpkg.io/en/getting-started) .
 
  - **Make** : for Windows, install with MinGW64. For Linux, typically installed by default.
 
  - **Vulkan SDK** : follow instructions at [Vulkan SDK](https://vulkan.lunarg.com/) to install if you want debug features
 
- ### Steps 

  - Ensure you have a C++20 compiler, Vcpkg, and Make.
 
  - Clone the repository
    `$ git clone https://github.com/platonvin/lum-al.git` 
 
  - Navigate to the project directory:
`$ cd lum-al`
  - Build library:
`$ make library -j4`
    - On Linux, GLFW may ask you to install multiple packages. Install them in advance:
`sudo apt install libxinerama-dev libxcursor-dev xorg-dev libglu1-mesa-dev pkg-config build-essential`.
     - Built library is placed into /lib. Use `-llumal` to link with it (do not forget to `-Llum-al/lib`)
 - use `make example` to build and run example-triangle.

see [src/al.hpp](src/al.hpp) for more info
