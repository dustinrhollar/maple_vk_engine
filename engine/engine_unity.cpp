
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

#include "utils/mstring.h"
#include "utils/vector_math.h"
#include "utils/hash_functions.h"

//~ Graphics API

#include "graphics/graphics.h"
#include "graphics/resources.h"
#include "graphics/renderer.h"

//~ Assets

#define CGLTF_IMPLEMENTATION
#include "assets/cgtlf.h"

// gltf converter
#include "assets/gltf_converter.h"
#include "assets/gltf_converter.cpp"

//~ Frame Info

#include "frame_info/frame_params.h"

//~ Platform Specific Code & Entry point

#include "graphics/dx11/maple_dx11.cpp"
#include "graphics/dx11/resources_dx11.cpp"
#include "graphics/dx11/renderer_dx11.cpp"

#include "platform/platform_entry.cpp"
