# Maple Engine

An in-development game engine that uses Vulkan for backend rendering. 

## Platform Support



## Installation



## Dependencies

One of the primary goals of the engine is to keep the number of dependencies to a minimum. However, there are some aspects of development that can be sped up considerably when using a third party library. Here is a list of dependencies for the engine

### ImGUI

[ImGUI](https://github.com/ocornut/imgui) is a UI library that is used for engine UI. Currently, ImGUI is compiled with the engine and is not compiled as an external library.

### stb Headers

Sean Barret's [collection](https://github.com/nothings/stb) of header only libraries are incredibly useful and compact. Maple uses `stb_image.h` to load images from disk.

### Vulkan Memory Allocator

The [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) is an incredibly robust framework for GPU memory management.

### cgltf 

Of the many glTF parsers I have seen, [cgltf](https://github.com/jkuhlmann/cgltf) is one of the best. It is compact and does exactly what you want it to do without all of the extra json parsing you get with other glTF libraries. 

### Vulkan

Vulkan is used as the backend graphics API. The binaries are not shipped with the engine, so it is on the user to have properly set up Vulkan on their platform. However, the required header files are included with the engine.

