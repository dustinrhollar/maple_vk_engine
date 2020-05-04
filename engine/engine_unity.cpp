
//~ Vulkan includes
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

// Unity build file for all engine code

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
//#include "mm/vulkan_allocator.h"
#include "mm/mm.h"


//~ Utility Headers

// jstring definitions
#define pstring_alloc(s)         palloc(s)
#define pstring_realloc(src, sz) prealloc((src), (sz))
#define pstring_free(p)          pfree(p)

#define tstring_alloc(s)   palloc(s)
#define tstring_realloc(s) prealloc(s)
#define tstring_free(p)    pfree(p)

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
#include "vk/splicer_vulkan.h"

//~ File IO Operations (OS Independent)
#include "io/file.h"
#include "io/file.cpp"

//~ Memory Manager Source Files
//#include "mm/stack_allocator.cpp"
#include "mm/linear_allocator.cpp"
#include "mm/free_list_allocator.cpp"
//#include "mm/pool_allocator.cpp"
#include "mm/proxy_allocator.cpp"
//#include "mm/vulkan_allocator.cpp"
#include "mm/mm.cpp"

//~ ECS Source Files
#include "ecs/entity.cpp"
#include "ecs/component.cpp"
#include "ecs/system.cpp"
#include "ecs/ecs.cpp"


//~ Vulkan Source Files
#include "vk/vulkan_functions.cpp"
#include "vk/splicer_vulkan.cpp"

//~ ENTRY POINT
#include "platform/entry.cpp"