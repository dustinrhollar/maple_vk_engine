

//~ Required Game level functions - must be declared here
// These are functions the engine needs to about related to the game.
// For example, once the engine is done setting up internal behaviors,
// it calls into "GameInit()" and when it is ready to render the next
// frame "GameUpdateAndRender"

// defined in platform.h
struct FrameInput;
struct frame_params;

// NOTE(Dustin): OLD FUNCTIONALITY
void GameInit();
void GameShutdown();
void GameUpdateAndRender(FrameInput input);
void FlagGameResize();

// NOTE(Dustin): NEW FUNCTIONALITY
void GameStageInit(frame_params* FrameParams);
void GameStageShutdown(frame_params* FrameParams);
void GameStageEntry(frame_params* FrameParams);


//~ Vulkan includes
#include <vulkan/vulkan.h>

//~ Required C++ Headers
//~ CLib  Headers
#include <assert.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Used for determining if a registered component
// inherits from IComponent
// needed for placement new
#include <new>
// NOTE(Dustin): Maybe can replace with hashtable
#include <set>
#include <new>
// NOTE(Dustin): Handle this better: Used for VK_CHECK_RESULT
#include <stdexcept>
// NOTE(Dustin): Is this still necessary?
#include <optional>

//~ External Libraries

// TODO(Dustin): Compile ImGui as a static lib
#define IMGUI_IMPLEMENTATION
#include <imgui/misc/single_file/imgui_single_file.h>

//~ Type System and Configuration
#include "utils/maple_types.h"
#include "engine_config.h"

//~ Memory Manager Headers
#include "mm/allocator.h"
//#include "mm/stack_allocator.h"
#include "mm/linear_allocator.h"
#include "mm/free_list_allocator.h"
//#include "mm/pool_allocator.h"
#include "mm/proxy_allocator.h"
#include "mm/vulkan_allocator.h"
#include "mm/mm.h"


//~ Utility Macros so that the utility functions use the Memory Manager

// Dynamic Array

#define parray_alloc(s)         palloc(s)
#define parray_realloc(src, sz) prealloc((src), (sz))
#define parray_free(p)          pfree(p)

#define tarray_alloc(s)   palloc(s)
#define tarray_realloc(s) prealloc(s)
#define tarray_free(p)    pfree(p)

// HashTable

#define ptable_alloc(s)         palloc(s)
#define ptable_realloc(src, sz) prealloc((src), (sz))
#define ptable_free(p)          pfree(p)

#define ttable_alloc(s)   palloc(s)
#define ttable_realloc(s) prealloc(s)
#define ttable_free(p)    pfree(p)

// jstring definitions

#define pstring_alloc(s)         palloc(s)
#define pstring_realloc(src, sz) prealloc((src), (sz))
#define pstring_free(p)          pfree(p)

#define tstring_alloc(s)   palloc(s)
#define tstring_realloc(s) prealloc(s)
#define tstring_free(p)    pfree(p)


//~ Utility Headers

#define MAPLE_HASHTABLE_IMPLEMENTATION
#define MAPLE_JSTRING_IMPLEMENTATION
#define MAPLE_HASH_FUNCTION_IMPLEMENTATION
#define MAPLE_DYNAMIC_ARRAY_IMPLEMENTATION
#define MAPLE_VECTOR_MATH_IMPLEMENTATION

#include "utils/dynamic_array.h"
#include "utils/hash_functions.h"
#include "utils/jstring.h"
#include "utils/hashtable.h"
#include "utils/jtuple.h"
#include "utils/vector_math.h"

//~ Platform Specfic Functions
#include "platform/platform.h"


//~ ECS Headers
#include "ecs/entity.h"
#include "ecs/component.h"
#include "ecs/system.h"
#include "ecs/ecs.h"

//~ Events

#include "events/engine_events.h"
#include "events/event_manager.h"

#include "events/event_manager.cpp"

//~ Vulkan Headers
// TODO(Dustin): Find a better solution to this.
// Lines 15271-15305 have been turned off in order to compile
// Lines 15313-15321
// Lines 15329-15330
// Lines 15334-15335
// Lines 15344
// Lines 15349
// Lines 15360
#define VMA_IMPLEMENTATION

#ifdef VK_USE_PLATFORM_WIN32_KHR
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#define LoadFunction LoadProcAddress

HMODULE VulkanLibrary;

#elif defined (linux) || defined (__unix__)

#include <dlfcn.h>
#define LoadFunction dlsym

void *VulkanLibrary;

#else
#error "Platform not supported!"
#endif

#include "vk/vulkan_functions.h"
#include <vma/vk_mem_alloc.h>
#include "vk/maple_vulkan.h"

//~ ImGui Vulkan Implementation

#include "ui/imgui_impl_vulkan.h"
#include "ui/imgui_impl_vulkan.cpp"

//~ File IO Operations (OS Independent)
#include "io/file.h"
#include "io/file.cpp"

//~ Memory Manager Source Files
//#include "mm/stack_allocator.cpp"
#include "mm/linear_allocator.cpp"
#include "mm/free_list_allocator.cpp"
//#include "mm/pool_allocator.cpp"
#include "mm/proxy_allocator.cpp"
#include "mm/vulkan_allocator.cpp"
#include "mm/mm.cpp"

//~ ECS Source Files
#include "ecs/entity.cpp"
#include "ecs/component.cpp"
#include "ecs/system.cpp"
#include "ecs/ecs.cpp"


//~ Vulkan Source Files
#include "vk/vulkan_functions.cpp"
#include "vk/maple_vulkan.cpp"

//~ Config Source
#include "config/config_parser.h"
#include "config/config_parser.cpp"

//~ Resource Managers

#include "resources/resource_allocator.h"
#include "resources/resource_allocator.cpp"

#define CGLTF_IMPLEMENTATION
#include "resources/cgltf.h"
#include "resources/resources.h"
#include "resources/asset.h"
#include "resources/mesh_converter.h"
#include "renderer/frontend.h"
#include "renderer/backend.h"
#include "frame_info/frame_params.h"

#include "frame_info/frame_params.cpp"
#include "resources/asset.cpp"
#include "resources/resources.cpp"
#include "resources/mesh_converter.cpp"
#include "renderer/frontend.cpp"
#include "renderer/backend.cpp"

//~ Fiber Implementation
#include "fibers/tagged_heap.h"
#include "fibers/tagged_heap.cpp"

//~ ENTRY POINT
#include "platform/entry.cpp"