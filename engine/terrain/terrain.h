#ifndef TERRAIN_H
#define TERRAIN_H

struct terrain_vertex
{
    vec2 Position;
    vec3 Normal;
    vec2 Uvs;
};

struct terrain_mesh
{
    u32 Width, Height;
    u32 VertexCount, IndexCount;
    
#if 0
    terrain_vertex         *Vertices;
    u32                   *Indices;
    
    BufferParameters       VertexBuffer;
    BufferParameters       IndexBuffer;
    
    VkPipelineLayout       PipelineLayout;
    VkPipeline             SolidPipeline;
    VkPipeline             WireframePipeline;
    
    r32                   *Heightmap;
    ImageParameters        HeightmapImage;
    
    VkDescriptorSetLayout  MaterialDescriptorLayout;
    VkDescriptorSet       *MaterialDescriptorSets;
    u32                    DescriptorCount;
    
    // terrain_mesh does not own the descriptor pool!
    VkDescriptorPool DescriptorPool;
#endif
};

void InitTerrain(terrain_mesh *terrain);
void DestroyTerrain(terrain_mesh *terrain);
void GenerateTerrainGrid(terrain_mesh *terrain, u32 width, u32 height);
void CreateTerrainVertexBuffers(terrain_mesh *terrain);
void CreateTerrainMaterial(terrain_mesh *terrain);
void AttachHeightmapToTerrain(terrain_mesh *terrain, r32 *heightmap, u32 width, u32 height);

void RenderTerrain(terrain_mesh *terrain, bool is_wireframe = false);
void RegenerateTerrain(terrain_mesh *terrain, terrain_settings &settings);



#endif //TERRAIN_H
