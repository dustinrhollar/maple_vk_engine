#ifndef TERRAIN_H
#define TERRAIN_H

struct terrain_mesh
{
    u32         Width;
    u32         Height;
    u32         VertexCount;
    u32         IndexCount;
    
    resource_id Pipeline;
    resource_id VertexBuffer;
    resource_id IndexBuffer;
    resource_id HeightmapTexture;
    
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


void CreateTerrain(asset_registry *AssetRegistry, terrain_settings *Settings);
void ResetTerrain(asset_terrain *Terrain, terrain_settings *Settings);
void DestroyTerrain(asset_terrain *Terrain);


void GenerateTerrainGrid(asset_terrain *terrain, u32 width, u32 height);
void CreateTerrainVertexBuffers(asset_terrain *terrain);
void CreateTerrainMaterial(asset_terrain *terrain);
void AttachHeightmapToTerrain(asset_terrain *Terrain, u32 TextureWidth, u32 TextureHeight);


#endif //TERRAIN_H
