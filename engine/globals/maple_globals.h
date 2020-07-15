
#ifndef ENGINE_GLOBALS_MAPLE_GLOBALS_H
#define ENGINE_GLOBALS_MAPLE_GLOBALS_H

#include "../mm/free_list_allocator.h"
#include "../mm/tagged_heap.h"
#include "../mm/pool_allocator.h"
#include "../utils/mstring.h"
#include "../utils/vector_math.h"
#include "../graphics/resources.h"
#include "../graphics/renderer.h"
#include "../assets/asset.h"

//~ Graphics dependent state

// defined in graphics-specific file
// ex. If Renderer is DX11, then is defined in renderer_dx11.cpp
extern renderer_t                GlobalRenderer;

// The resource registry is graphics dependent, so
// must be declared as a pointer
// ex. If Renderer is DX11, then is defined in resources_dx11.cpp
extern struct resource_registry *GlobalResourceRegistry;

//~ Global memory

// global memory 
// - use palloc<Type>(GlobalMemory, Count) for allocations
extern free_allocator            GlobalMemory;
// Tagged Heap for frame - local memory
// - use halloc<Type>(TaggedBlock, Count) for allocations
extern tagged_heap               GlobalTaggedHeap;
// String arena for mstring
extern free_allocator            GlobalStringArena;

//~ Asset Registry

extern asset_registry            GlobalAssetRegistry;

//~ Create Info structs for initializing globals

struct global_memory_create_info
{
    u64 Size;
};

struct global_tagged_heap_create_info
{
    u64 Size;
    u64 SizePerBlock;
    u32 ActiveTags;
};

struct global_string_arena_create_info
{
    u64 Size;
};

struct global_resource_registry_create_info
{
    u64 MaxCount;
};

struct global_asset_registry_create_info
{
    u64 MaxCount;
};

struct global_renderer_create_info
{
    u32              Width;
    u32              Height;
    u32              RefreshRate;
    platform_window *Window;
};

struct globals_create_info
{
    global_memory_create_info            GlobalMemory;
    global_tagged_heap_create_info       TaggedHeap;
    global_string_arena_create_info      StringArena;
    global_resource_registry_create_info ResourceRegistry;
    global_asset_registry_create_info    AssetRegistry;
    global_renderer_create_info          Renderer;
    
    // ImGui is initialized internally
};

void InitializeGlobals(globals_create_info *CreateInfo);
void FreeGlobals();

#endif //ENGINE_GLOBALS_MAPLE_GLOBALS_H
