
# Lum-al
abstraction layer on top of Vulkan, originally designed as a part of [Lum engine](https://github.com/platonvin/lum), but extracted to be used in other projects

this is very thin C++ Vulkan abstraction, built for usecase when objects (renderpasses, pipelines, framebuffers, descriptors) are mostly defined at initializon time

see [examples/triangle.cpp](examples/triangle.cpp) and [src/al.hpp](src/al.hpp) for more info