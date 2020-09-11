#ifndef PLATFORM_WORLD_POLYGONAL_WORLD_H
#define PLATFORM_WORLD_POLYGONAL_WORLD_H

typedef struct terrain
{
    pipeline Pipeline;
    pipeline NormalVis;
    
    // Voxel Terrain Render Component
    render_component RenderInfo;
    // Vertex Data is accumulated into here
    upload_buffer    VertexUploadBuffer;
    upload_buffer    IndexUploadBuffer;
    
    // Density of the mesh
    u32              Width;
    u32              Height;
    
    // terrain transforms
    vec3             Scale;
    vec3             Position;
    quaternion       Rotation;
    
    // Heightmap to bind
    image            Heightmap;
    // TODO(Dustin): Descriptors?
    
    descriptor_layout TerrainLayout;
    descriptor_set    TerrainSet;
    
} terrain;

typedef struct world
{
    command_pool CommandPool;
    command_list CommandList;
    
    terrain      Terrain;
    
} world;

void world_init(world *World);
void world_free(world *World);

#if 0
void world_draw(world *World);
#endif

#endif //PLATFORM_WORLD_POLYGONAL_WORLD_H
