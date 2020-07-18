#ifndef ENGINE_GRAPHICS_RENDERER_H
#define ENGINE_GRAPHICS_RENDERER_H

typedef struct renderer* renderer_t;

#if 0
struct renderer
{
    resource_id Device;
    resource_id RenderTarget;
    
    // Temporary Code
    resource_id VertexBuffer;
    resource_id SimplePipeline;
    
    maple_ui    MapleUi;
};
#else

#endif


void RendererInit(free_allocator    *Allocator,
                  resource_registry *Registry,
                  window_t           Window,
                  u32                Width,
                  u32                Height,
                  u32                RefreshRate);
void RendererShutdown(free_allocator *Allocator);

void RendererResize(resource_registry *Registry);
void RendererEntry(frame_params *FrameParams);

#endif //ENGINE_GRAPHICS_RENDERER_H
