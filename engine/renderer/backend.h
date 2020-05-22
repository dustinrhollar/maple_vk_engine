#ifndef ENGINE_RENDERER_BACKEND_H
#define ENGINE_RENDERER_BACKEND_H

struct gpu_begin_frame_info
{
    // Clear Color Info
    VkClearColorValue Color;
    //vec4 Color;
    
    // Depth Stencil Info
    bool HasDepth;
    r32  Depth;
    u32  Stencil;
};

struct gpu_end_frame_info
{
};

struct gpu_vertex_buffer_create_info
{
    VkBufferCreateInfo       BufferCreateInfo;
    VmaAllocationCreateInfo  VmaCreateInfo;
    VkBuffer                *Buffer;
    VmaAllocation           *Allocation;
    void                    *Data;
    VkDeviceSize             Size;
};

struct gpu_index_buffer_create_info
{
    VkBufferCreateInfo       BufferCreateInfo;
    VmaAllocationCreateInfo  VmaCreateInfo;
    VkBuffer                *Buffer;
    VmaAllocation           *Allocation;
    void                    *Data;
    VkDeviceSize             Size;
};

struct gpu_set_scissor_info
{
    VkExtent2D Extent;
    i32        XOffset, YOffset;
};

struct gpu_set_viewport_info
{
    r32 Width, Height;
    r32 X, Y;
};

struct gpu_image_create_info
{
};

struct gpu_draw_indexed_info
{
};

struct gpu_draw_info
{
};

struct gpu_descriptor_create_info
{
};

struct gpu_descriptor_update_info
{
};

struct gpu_descriptor_bind_info
{
};

struct gpu_command_buffer_begin_info
{
};

struct gpu_command_buffer_end_info
{
};

struct gpu_render_pass_begin_info
{
};

struct gpu_render_pass_end_info
{
};

enum gpu_command_type
{
    GpuCommand_BeginFrame,
    GpuCommand_EndFrame,
    
    //  Scissor/Viewport Commands
    GpuCommand_SetScissor,
    GpuCommand_SetViewport,
    
    // Upload Resources
    GpuCommand_UploadVertexBuffer,
    GpuCommand_UploadIndexBuffer,
    GpuCommand_UploadImage,
    
    // Drawing
    GpuCommand_Draw,         // draw meshes w/o indices
    GpuCommand_DrawIndexed,  // draw indexed meshes
    
    // Descriptors
    GpuCommand_CreateDescriptor, // NOTE(Dustin): Currently done in Resources...
    GpuCommand_UpdateDescriptor,
    GpuCommand_CopyDescriptor,
    GpuCommand_BindDescriptor,
    
    // Command Buffer
    GpuCommand_BeginCommandBuffer,
    GpuCommand_EndCommandBuffer,
    
    // Render Pass
    GpuCommand_BeginRenderPass,
    GpuCommand_EndRenderPass,
};

struct gpu_command
{
    gpu_command_type Type;
    void *Data;
};

void GpuStageInit(frame_params  *FrameParams);
void GpuStageEntry(frame_params *FrameParams);
void GpuStageShutdown(frame_params* FrameParams);

// This will return the main renderpass used in the backend.
// This is a HACK because there is not currently a proper system
// in place to handle multiple render passes...
VkRenderPass GetPrimaryRenderPass();

#endif //ENGINE_RENDERER_BACKEND_H
