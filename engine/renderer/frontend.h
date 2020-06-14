#ifndef ENGINE_RENDERER_FRONTEND_H
#define ENGINE_RENDERER_FRONTEND_H

struct frame_params;

struct render_set_scissor_info
{
    VkExtent2D Extent;
    i32        XOffset, YOffset;
};

struct render_set_viewport_info
{
    r32 Width, Height;
    r32 X, Y;
};


struct render_bind_pipeline_info
{
    resource_id_t PipelineId;
};

struct render_bind_descriptor_set
{
    resource_id_t PipelineId;
    resource_id_t DescriptorId;
    u32           FirstSet; // set number of the first set being bound
    u32          *DynamicOffsets;
    u32           DynamicOffsetsCount;
};

struct render_draw_command
{
    object_shader_data ObjectShaderData;
    
    resource_id_t     *VertexBuffers;
    u64               *Offsets;
    u32                VertexBuffersCount;
    
    bool               IsIndexed;
    resource_id_t      IndexBuffer;
    
    // will be either vertex count or index count depending if
    // the draw is indexed
    u32                Count;
    
    asset_id_t         Material;
};

struct render_draw_dev_gui
{
    maple_dev_gui *DevGui;
};

enum render_command_type
{
    RenderCmd_Draw,
    
    // NOTE(Dustin): Assets are loaded by the engine or the platform layer
    // should no longer be called by the game layer.
    RenderCmd_LoadAsset,
    
    // NOTE(Dustin): Probably shouldn't be called from the game layer,
    // will allow it for now.
    RenderCmd_SetViewport,
    RenderCmd_SetScissor,
    
    // NOTE(Dustin): No longer should be called from the game layer
    RenderCmd_BindPipeline,
    RenderCmd_BindDescriptorSet,
    
    // Draw Ui
    RenderCmd_DrawDevGui,
};

struct render_command
{
    render_command_type  Type;
    void                *Data;
};

void RenderStageInit(frame_params *FrameParams);
void RenderStageEntry(frame_params *FrameParams);

#endif //ENGINE_RENDERER_FRONTEND_H
