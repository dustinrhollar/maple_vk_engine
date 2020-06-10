# Maple Engine

An in-development game engine that uses Vulkan for backend rendering. 

## Platform Support

The primary platform for this engine is Windows. There are plans to extend full support to Linux, but the X11 platform file is not yet complete enough to be supported. 

## Installation

If on Windows, open the command prompt, navigate to directory, and execute the following commands. 

1. Compile the engine (and example project). This command also preps the build directory.
```
run build
```

2. Copy resource files over to the build directory. 
```
run rsrc
```

3. Compile Shaders.
```
run shad
```

4. Compile any external dependencies. This will compile the reflection generator tool.
```
run build_ext
```

5. Run the example project.
```
run
```

## Engine Usage

Maple uses the unity build system where source is included into a single `*.cpp` file. The main "unity" file is located in the top level directory and is named `unity.cpp`. This file incudes:
1. The Engine unity file
2. The Application unity file

By default, the Application unity file will be the example project. To include one's own project, edit `unity.cpp` to include the desired unity file.

## Engine Features

Rught now, the engine does not contain many flashy features worth showing off. In fact, you could probably call Maple the most robust triangle renderer of all time. Nonetheless, there are some interesting features working behind the scenes.

### Memory Management



### Entity-Component-System


### Rendering Architecture


### Maple Config Langauge


## Dependencies

One of the primary goals of the engine is to keep the number of dependencies to a minimum. However, there are some aspects of development that can be sped up considerably when using a third party library. Here is a list of dependencies for the engine

### ImGUI

[ImGUI](https://github.com/ocornut/imgui) is a UI library that is used for engine UI. Currently, ImGUI is compiled with the engine and is not compiled as an external library.

### stb Headers

Sean Barret's [collection](https://github.com/nothings/stb) of header only libraries are incredibly useful and compact. Maple uses `stb_image.h` to load images from disk.

### Vulkan Memory Allocator

The [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) is an incredibly robust framework for GPU memory management.

### cgltf 

Of the many glTF parsers I have seen, [cgltf](https://github.com/jkuhlmann/cgltf) is one of the best. It is compact and does exactly what you want it to do without all of the extra json fluff you get with other glTF libraries. 

### Vulkan

Vulkan is used as the backend graphics API. The binaries are not shipped with the engine, so it is on the user to have properly set up Vulkan on their platform. However, the required header files are included with the engine.
