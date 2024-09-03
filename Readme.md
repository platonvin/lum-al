
# Lum-al
**Lum-al** is abstraction layer on top of Vulkan, originally designed as a part of [**Lum engine**](https://github.com/platonvin/lum), but extracted to be used in other projects

this is very thin C++ Vulkan abstraction, built for usecase when objects (renderpasses, pipelines, framebuffers, descriptors) are mostly defined at initializon time

to prevent your language server from parsing files it should not, define _LANG_SERVER (for example, with -D_LANG_SERVER argument) for it

[examples/triangle.cpp](examples/triangle.cpp) contains example to draw simple triangle with 2 subpasses - main shading (writes to intermediate image) and posteffect (reads from intermediate image via supass input and then writes to swapchain)

use `make` to build library and run examples. Built library is placed into /lib. Use -llumal to link with it (do not forget to -Llum-al/lib)

see [src/al.hpp](src/al.hpp) for more info