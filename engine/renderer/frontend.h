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
    primitive         *PrimitivesToDraw;
    u32                PrimitivesCount;
    
    object_shader_data ObjectShaderData;
    
    // Hide?
#if 0
    resource_id_t    *VertexBuffers;
    u64              *Offsets;
    u32               VertexBuffersCount;
    
    bool             IsIndexed;
    resource_id_t    IndexBuffer;
    
    
    // will be either vertex count or index count depending if
    // the draw is indexed
    u32 Count;
#endif
};

enum render_command_type
{
    RenderCmd_Draw,
    RenderCmd_LoadAsset,
    
    RenderCmd_SetViewport,
    RenderCmd_SetScissor,
    
    RenderCmd_BindPipeline,
    RenderCmd_BindDescriptorSet,
};

struct render_command
{
    render_command_type  Type;
    void                *Data;
};

void RenderStageInit(frame_params *FrameParams);
void RenderStageEntry(frame_params *FrameParams);

#endif //ENGINE_RENDERER_FRONTEND_H
