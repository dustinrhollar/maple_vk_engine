
//~ Utils and C Standard Library
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../platform/utils/maple_types.h"

#define MAPLE_VECTOR_MATH_IMPLEMENTATION
#include "../platform/utils/vector_math.h"
#include "../platform/utils/camera.h"

#include "voxel_pfn.h"

//~ Platform and Graphics headers

#include "../platform/platform/platform.h"
// TODO(Dustin): Remove Vulkan header...
#include "../graphics/vulkan/vulkan.h"
#include "../graphics/maple_graphics.h"
#include "../platform/platform/library_loader.h"
#include "../platform/frame_params/frame_params.h"
#include "../platform/world/polygonal_world.h"

#include "../platform/mm/memory.h"
#include "../platform/mm/memory.c"

//~ Game Headers

#include "heightmap.h"
#include "simplex_noise.h"
#include "gaussian_blur.h"

//~ Game Source

#include "heightmap.c"
#include "simplex_noise.c"
#include "gaussian_blur.c"
#include "terrain.c"
#include "voxel_entry.c"
