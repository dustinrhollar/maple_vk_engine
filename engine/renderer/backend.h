#ifndef ENGINE_RENDERER_BACKEND_H
#define ENGINE_RENDERER_BACKEND_H

struct gpu_vertex_buffer_create_info
{
};

struct gpu_index_buffer_create_info
{
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
    // Upload Resources
    GpuCommand_UploadVertexBuffer,
    GpuCommand_UploadIndexBuffer,
    GpuCommand_UploadImage,
    
    // Drawing
    GpuCommand_Draw,         // draw meshes w/o indices
    GpuCommand_DrawIndexed,  // draw indexed meshes
    
    // Descriptors
    GpuCommand_CreateDescriptor,
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

void GpuStageInit(frame_params FrameParams);
void GpuStageEntry(frame_params FrameParams);


#endif //ENGINE_RENDERER_BACKEND_H
