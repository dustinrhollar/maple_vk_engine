#ifndef ENGINE_RENDERER_BACKEND_H
#define ENGINE_RENDERER_BACKEND_H

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

struct gpu_begin_frame_info
{
    // Clear Color Info
    VkClearColorValue Color;
    
    // Depth Stencil Info
    bool HasDepth;
    r32  Depth;
    u32  Stencil;
    
    // Global Frame Data to set
    global_shader_data GlobalShaderData;
};

struct gpu_end_frame_info
{
};

struct gpu_vertex_buffer_create_info
{
    VkBufferCreateInfo       BufferCreateInfo;
    VmaAllocationCreateInfo  VmaCreateInfo;
    
    BufferParameters        *BufferParams;
    
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

struct gpu_image_create_info
{
    jstring              Filename;
    
    VkFilter             MagFilter;
    VkFilter             MinFilter;
    
    VkSamplerAddressMode AddressModeU;
    VkSamplerAddressMode AddressModeV;
    VkSamplerAddressMode AddressModeW;
    
    ImageParameters     *Image;
};

// TODO(Dustin): Allow for instancing
struct gpu_draw_info
{
    VkBuffer         *VertexBuffers;
    u64              *Offsets;
    u32               VertexBuffersCount;
    
    bool             IsIndexed;
    VkBuffer         IndexBuffer;
    
    // will be either vertex count or index count depending if
    // the draw is indexed
    u32 Count;
};

struct gpu_bind_pipeline_info
{
    VkPipeline Pipeline;
};

struct gpu_descriptor_create_info
{
};

struct descriptor_write_info
{
    union
    {
        resource_id_t    BufferId;
        resource_id_t    TextureId;
    };
    
    resource_id_t    DescriptorId;
    u32              DescriptorBinding;
    VkDescriptorType DescriptorType;
};


struct gpu_descriptor_update_info
{
    descriptor_write_info       *WriteInfos;
    u32                          WriteInfosCount;
};

struct gpu_descriptor_set_bind_info
{
    VkPipelineLayout PipelineLayout;
    resource_id_t    DescriptorId;
    u32             *DynamicOffsets;
    u32              DynamicOffsetsCount;
    u32              FirstSet;
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

struct gpu_update_buffer_info
{
    resource_id_t  Uniform;
    
    // Offset into the uniform buffer
    u32            BufferOffset = 0;
    
    void          *Data;
    u32            DataSize;
};

enum gpu_command_type
{
    GpuCmd_BeginFrame,
    GpuCmd_EndFrame,
    
    //  Scissor/Viewport Commands
    GpuCmd_SetScissor,
    GpuCmd_SetViewport,
    
    // Upload Resources
    GpuCmd_UploadVertexBuffer,
    GpuCmd_UploadIndexBuffer,
    GpuCmd_UploadImage,
    
    // Update Resources
    GpuCmd_UpdateBuffer,
    
    // Drawing
    GpuCmd_Draw,
    
    // Descriptors
    GpuCmd_CreateDescriptor, // NOTE(Dustin): Currently done in Resources...
    GpuCmd_UpdateDescriptor,
    GpuCmd_CopyDescriptor,
    
    // Binding Resources
    GpuCmd_BindPipeline,
    GpuCmd_BindDescriptorSet,
    
    // Cmd Buffer
    GpuCmd_BeginCommandBuffer,
    GpuCmd_EndCommandBuffer,
    
    // Render Pass
    GpuCmd_BeginRenderPass,
    GpuCmd_EndRenderPass,
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
