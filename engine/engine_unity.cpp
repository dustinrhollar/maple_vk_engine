
// C Headers
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h.>

// Maple Engien Source
#include "utils/maple_types.h"

// Platform file
#include "platform/platform.h"

// Memory Management syste
#include "mm/free_list_allocator.h"
#include "mm/free_list_allocator.cpp"

#include "mm/tagged_heap.h"
#include "mm/tagged_heap.cpp"

#include "mm/pool_allocator.h"
#include "mm/pool_allocator.cpp"

#include "mm/mm.h"

// Utilities
#define MAPLE_MSTRING_IMPLEMENTATION
#define MAPLE_VECTOR_MATH_IMPLEMENTATION

#include "utils/mstring.h"
#include "utils/vector_math.h"

// File I/O
//#include "file/file.h"
//#include "file/file.cpp"

// Graphics API
#include "graphics/graphics.h"
#include "graphics/resources.h"
#include "graphics/renderer.h"

// Resources

// Platform Specific Code & Entry point
#include "platform/platform_entry.cpp"

#include "graphics/dx11/maple_dx11.cpp"
#include "graphics/dx11/resources_dx11.cpp"
#include "graphics/dx11/renderer_dx11.cpp"