
// C Headers
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

// Maple Engien Source
#include "utils/maple_types.h"

// Platform file
#include "platform/platform.h"

// Memory Management syste
#include "mm/free_list_allocator.h"
#include "mm/free_list_allocator.c"

#include "mm/tagged_heap.h"
#include "mm/tagged_heap.c"

// Utilities
#define MAPLE_MSTRING_IMPLEMENTATION

#include "utils/mstring.h"

// Entry point
#include "platform/platform_entry.c"
