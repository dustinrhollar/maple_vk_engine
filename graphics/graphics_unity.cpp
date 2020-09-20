//-------------------------------------------------
// HEADERS
//-------------------------------------------------

//~ C Headers

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h.>

// NOTE(Dustin): Used for queue and graphics families
#include <set>
#include <optional>

//~ Type System & Utils

#define STB_DS_IMPLEMENTATION
#define USE_MAPLE_MSTR_IMPLEMENTATION
#define MAPLE_VECTOR_MATH_IMPLEMENTATION

#include "../platform/utils/maple_types.h"
#include "vulkan/vulkan.h"

#include "../platform/platform/win32/assetsys.h"
#include "../platform/platform/platform.h"
#include "platform.h"

platform *Platform;

//~ Vulkan Library

// TODO(Dustin): Find a better solution to this.
// Lines 15271-15305 have been turned off in order to compile
// Lines 15313-15321
// Lines 15329-15330
// Lines 15334-15335
// Lines 15344
// Lines 15349
// Lines 15360
#define VMA_IMPLEMENTATION

#include "vulkan_functions.h"        // imported functions
#include "vma/vk_mem_alloc.h" // gpu memory allocator
#include "maple_vk.h"            // API wrapper

// Win32 implementation
// TODO(Dustin): I dont think this is necessary to have here, but leave until i get a compliation
#ifdef VK_USE_PLATFORM_WIN32_KHR
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#define LoadFunction LoadProcAddress

HMODULE VulkanLibrary;

#endif

typedef struct 
{
    struct memory          *Memory;
    struct renderer        *Renderer;
    vulkan_core             VkCore;
} globals;

extern globals *Core;

#include "../platform/mm/memory.h"
#include "mm.h"
#include "../platform/utils/stb_ds.h"
#include "../platform/utils/mstr.h"
#include "../platform/utils/vector_math.h" 

#include "dynamic_uniform_buffer.h"
#include "uniform_buffer.h"
#include "maple_graphics.h"
#include "renderer.h"

//-------------------------------------------------
// SOURCE
//-------------------------------------------------

#include "../platform/mm/memory.c"

#include "vulkan_functions.cpp"
#include "maple_vk.cpp"

#include "dynamic_uniform_buffer.c"
#include "uniform_buffer.c"
#include "renderer.c"
#include "maple_graphics.cpp"

#include "graphics_win32.cpp"
