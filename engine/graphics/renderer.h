#ifndef ENGINE_GRAPHICS_RENDERER_H
#define ENGINE_GRAPHICS_RENDERER_H

typedef struct renderer* renderer_t;


struct terrain_settings
{
    bool    RecreateTerrain;
    bool    SaveHeightmap;
    bool    TerrainMeshUpdated;
    bool    HeightmapUpdated;
    
    // Terrain Settings
    u32     TerrainWidth;
    u32     TerrainHeight;
    
    // Simple Sim. Settings
    u32     HeightmapWidth;
    u32     HeightmapHeight;
    u32     NumberOfOctaves;
    r32     Persistence;
    r32     Low;
    r32     High;
    r32     Exp;
    
    // Thermal Erosion Settings
    bool    ThermalEnabled;
    u32     ThermalNumIterations;
    
    // Inverse Thermal Settings
    bool    InverseThermalEnabled;
    u32     InverseThermalNumIterations;
    
    // Hydraulic Settings
    bool    HydraulicEnabled;
    u32     HydraulicNumIterations;
    r32     RainConstant;
    r32     SolubilityConstant;
    r32     EvaporationCoefficient;
    r32     SedimentTransferMaxCoefficient;
    
    mstring SaveFilename;
};

struct maple_ui
{
    terrain_settings TerrainSettings;
};

struct renderer
{
    resource_id Device;
    resource_id RenderTarget;
    
    // Temporary Code
    resource_id VertexBuffer;
    resource_id SimplePipeline;
    
    maple_ui    MapleUi;
};

void RendererInit(renderer_t        *Renderer,
                  free_allocator    *Allocator,
                  resource_registry *Registry,
                  window_t           Window,
                  u32                Width,
                  u32                Height,
                  u32                RefreshRate);
void RendererShutdown(renderer_t     *Renderer,
                      free_allocator *Allocator);

void RendererResize(renderer_t Renderer, resource_registry *Registry);
void RendererEntry(renderer_t Renderer, frame_params *FrameParams);

#endif //ENGINE_GRAPHICS_RENDERER_H
