#ifndef ENGINE_RESOURCES_H
#define ENGINE_RESOURCES_H

typedef i64 resource_id_t;

struct dynamic_buffer_create_info;

// Pool/Linear Allocator for Dynamic UniformBuffers
namespace mm
{
    struct dyn_uniform_pool
    {
        u32 Alignment;
        u32 ElementSize;
        u32 Offset;
        
        BufferParameters Buffer;
    };
    
    void InitDynUniformPool(dyn_uniform_pool *Pool, dynamic_buffer_create_info *Info);
    void FreeDynUniformPool(dyn_uniform_pool *Pool);
    void AllocDynUniformPool(dyn_uniform_pool *Pool, void *Data, u32 Offset, bool IsMapped);
    void ResetDynUniformPool(dyn_uniform_pool *Pool, u32 Offset);
}; // mm

struct dyn_uniform_template
{
    u32 Alignment;
    u32 ElementSize;
    u32 Size;
    u32 Offset;
};

struct descriptor_layout_create_info
{
    u32                           BindingsCount;
    VkDescriptorSetLayoutBinding *Bindings;
};

struct descriptor_create_info
{
    u32            SetCount;
    resource_id_t *DescriptorLayouts;
};

struct buffer_create_info
{
    // if PersistentlyMapped is set to true and
    // Properties contains the VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    // then the buffer will be persistently mapped.
    u64                         BufferSize;
    bool                        PersistentlyMapped;
    VkBufferUsageFlags          Usage;
    VmaMemoryUsage              MemoryUsage;
    VmaAllocationCreateFlagBits MemoryFlags;
    
    // needed for Vertex/Index Buffers they will emit gpu commands
    frame_params               *FrameParams;
};

struct dynamic_buffer_create_info
{
    // if PersistentlyMapped is set to true and
    // Properties contains the VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    // then the buffer will be persistently mapped.
    u64                         ElementStride;
    u64                         ElementCount;
    bool                        PersistentlyMapped;
    VkBufferUsageFlags          Usage;
    VmaMemoryUsage              MemoryUsage;
    VmaAllocationCreateFlagBits MemoryFlags;
    
    // needed for Vertex/Index Buffers they will emit gpu commands
    frame_params               *FrameParams;
};

struct descriptor_write_info
{
    resource_id_t    BufferId;
    resource_id_t    DescriptorId;
    u32              DescriptorBinding;
    VkDescriptorType DescriptorType;
};

struct descriptor_update_write_info
{
    descriptor_write_info *WriteInfos;
    u32                   WriteInfosCount;
};

struct uniform_update_info
{
    
};

struct image_create_info
{
};

struct shader_file_create_info
{
    jstring               Filename;
    VkShaderStageFlagBits ShaderStage;
};

struct pipeline_create_info
{
    shader_file_create_info *Shaders;
    u32                      ShadersCount;
    
    VkPipelineVertexInputStateCreateInfo VertexInputInfo;
    
    VkViewport *Viewport;
    VkRect2D   *Scissor;
    u32         ScissorCount;
    u32         ViewportCount;
    
    // rasterizer
    VkPolygonMode PolygonMode;
    VkFrontFace   FrontFace;
    
    // Multisample
    bool                  HasMultisampling;
    VkSampleCountFlagBits MuliSampleSamples;
    
    // Depth/Stencil
    bool HasDepthStencil;
    
    // LayoutCreateInfo
    VkPushConstantRange   *PushConstants;
    u32                    PushConstantsCount;
    resource_id_t         *DescriptorLayoutIds;
    u32                    DescriptorLayoutsCount;
    
    VkRenderPass           RenderPass;
    
    // TODO(Dustin): Might want to expose subpasses?
};

struct vertex_buffer_create_info
{
    u64   Size;
    void *Data;
};

struct index_buffer_create_info
{
    u64   Size;
    void *Data;
};

struct update_buffer_info
{
    resource_id_t  BufferId;
    void          *Data;
    u32            DataSize;
};

enum resource_type
{
    Resource_DescriptorSetLayout,
    Resource_DescriptorSet,
    Resource_DescriptorSetWriteUpdate,
    Resource_DescriptorSetUpdate,
    Resource_VertexBuffer,
    Resource_IndexBuffer,
    Resource_UniformBuffer,
    Resource_DynamicUniformBuffer,
    Resource_Image,
    Resource_Pipeline,
    
    Resource_UpdateBuffer,
};

struct resource_descriptor_set_layout
{
    VkDescriptorSetLayout DescriptorLayout;
};

struct resource_descriptor_set
{
    DescriptorSetParameters *DescriptorSets;
    u32                      DescriptorSetsCount;
};

struct resource_vertex_buffer
{
    BufferParameters Buffer;
};

struct resource_index_buffer
{
    BufferParameters Buffer;
};

struct resource_image
{
    ImageParameters Image;
};

struct resource_pipeline
{
    //shader_data ShaderData;
    
    VkPipelineLayout Layout;
    VkPipeline       Pipeline;
};

struct resource_buffer
{
    BufferParameters  *Buffers;
    u32                BufferCount;
    bool               PersistentlyMapped;
};

struct resource_dynamic_buffer
{
    mm::dyn_uniform_pool *Pools;
    u32                   PoolsCount;
    bool                  PersistentlyMapped;
};

struct resource
{
    resource_id_t Id;
    resource_type Type;
    
    union
    {
        resource_descriptor_set_layout DescriptorLayout;
        resource_descriptor_set        DescriptorSet;
        resource_vertex_buffer         VertexBuffer;
        resource_index_buffer          IndexBuffer;
        resource_buffer                UniformBuffer;
        resource_dynamic_buffer        DynamicUniformBuffer;
        resource_image                 Image;
        resource_pipeline              Pipeline;
    };
};

namespace mresource
{
    void Init(frame_params *FrameParams);
    void Free(frame_params *FrameParams);
    
    resource_id_t Load(frame_params *FrameParams, resource_type Type, void *Data);
    
    resource GetResource(resource_id_t ResourceId);
    
    // The client should not directly call this function. Instead, create a
    // GpuCmd_Updatebuffer. It is not until the Gpu Stage that the ImageIndex is
    // known, so the Gpu stage has to issue the call to the resource manager to
    // actually update the uniform
    void UpdateUniform(resource_id_t Uniform, void *Data, u64 Size, u32 ImageIdx, u32 Offset = 0);
    
    // used for DynamicUniformBuffers to reset their internal allocator
    // Do not call directly! A Uniform reset should be done through the command
    // Gpu_ResetUniformBuffer
    void ResetDynamicUniformOffset(resource_id_t Uniform, u32 ImageIdx);
    
    // Returns a template for the requested dynamic uniform. This contains data
    // for the element stride and alignment, and an offset. This is necessary
    // for binding descriptors that use a dynamic uniform buffer and for updating
    // buffer data.
    dyn_uniform_template GetDynamicUniformTemplate(resource_id_t Uniform);
    
    // Will retrieve the next offset into the dynamic uniform buffer and update
    // the offset for the next request.
    i64 DynUniformGetNextOffset(dyn_uniform_template *DynUniformTemplate);
};

#endif //RESOURCES_H
