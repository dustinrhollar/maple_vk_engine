
//~ C Headers

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h.>

//~ Type System

#include "utils/maple_types.h"

//~ Platform file

#include "platform/platform.h"
#include "globals/maple_globals.h"

//~ Memory Management syste

#include "mm/free_list_allocator.h"
#include "mm/free_list_allocator.cpp"

#include "mm/tagged_heap.h"
#include "mm/tagged_heap.cpp"

#include "mm/pool_allocator.h"
#include "mm/pool_allocator.cpp"

#include "mm/mm.h"

//~ Utilities

#define MAPLE_MSTRING_IMPLEMENTATION
#define MAPLE_VECTOR_MATH_IMPLEMENTATION
#define MAPLE_HASH_FUNCTIONS_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define CGLTF_IMPLEMENTATION

#include "utils/hash_functions.h"
#include "utils/mstring.h"
#include "utils/vector_math.h"
#include "graphics/stb_image.h"
#include "assets/cgtlf.h"

//~ UI headers

#include "ui/imgui/imgui.h"
#include "ui/ui.h"
#include "ui/maple_ui_settings.h"

//~ Graphics API

#include "graphics/graphics.h"
#include "graphics/resources.h"
#include "graphics/renderer.h"

//~ Assets

// Terrain
#include "terrain/open_simplex_noise.h"
#include "terrain/terrain_generator.h"
#include "terrain/terrain.h"

// gltf converter
#include "assets/gltf_converter.h"
#include "assets/gltf_converter.cpp"

// asset manager
#include "assets/asset.h"
#include "assets/asset.cpp"

// asset loaders
#include "assets/asset_loader.h"
#include "assets/asset_loader.cpp"

// terrain source
#include "terrain/terrain_generator.cpp"
#include "terrain/terrain.cpp"

//~ Cameras

#include "camera/simple_camera.h"
#include "camera/simple_camera.cpp"

//~ Renderer API

#include "graphics/render_command.h"

//~ Frame Info

#include "frame_info/frame_params.h"
#include "frame_info/frame_params.cpp"

//~ Platform Specific Code & Entry point

#include "ui/win32/imgui_impl_win32.h"

// maple_dx11 defines the renderer struct
#include "graphics/dx11/maple_dx11.cpp"
#include "graphics/dx11/resources_dx11.cpp"
#include "graphics/dx11/renderer_dx11.cpp"

#include "ui/maple_ui.cpp"

#include "globals/maple_globals.cpp"
#include "platform/platform_entry.cpp"

// NOTE(Dustin): Not sure why, but including imgui above the entry point
// produces an error with timeBeginPeriod
// imgui
#include "ui/imgui/imgui.cpp"
#include "ui/imgui/imgui_demo.cpp"
#include "ui/imgui/imgui_draw.cpp"
#include "ui/imgui/imgui_widgets.cpp"
