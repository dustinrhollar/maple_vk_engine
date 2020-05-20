#ifndef ENGINE_RESOURCES_H
#define ENGINE_RESOURCES_H

typedef i64 resource_id_t;

struct descriptor_layout_create_info
{
    u32                           BindingsCount;
    VkDescriptorSetLayoutBinding *Bindings;
};

struct descriptor_create_info
{
    u32                    SetCount;
    resource_id_t *DescriptorLayouts;
};

struct buffer_create_info
{
    // if PersistentlyMapped is set to true and
    // Properties contains the VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    // then the buffer will be persistently mapped.
    u64                   SizePerBuffer;
    u32                   BufferCount;
    bool                  PersistentlyMapped;
    VkBufferUsageFlags    Usage;
    VmaMemoryUsage        Properties;
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

enum resource_type
{
    Resource_DescriptorSetLayout,
    Resource_DescriptorSet,
    Resource_VertexBuffer,
    Resource_IndexBuffer,
    Resource_UniformBuffer,
    Resource_DynamicUniformBuffer,
    Resource_Image,
    Resource_Pipeline,
};

struct resource_descriptor_set_layout
{
    VkDescriptorSetLayout DescriptorLayout;
};

struct resource_descriptor_set
{
    DescriptorSetParameters *DescriptorSets;
    u32                     DescriptorSetsCount;
};

struct resource_buffer
{
    BufferParameters  *Buffers;
    u32                BufferCount;
    bool              PersistentlyMapped;
};

struct resource_image
{
    ImageParameters Image;
};

struct resource_pipeline
{
    VkPipelineLayout Layout;
    VkPipeline       Pipeline;
};

struct resource
{
    resource_id_t Id;
    resource_type Type;
    
    union
    {
        resource_descriptor_set_layout DescriptorLayout;
        resource_descriptor_set        DescriptorSet;
        resource_buffer                VertexBuffer;
        resource_buffer                IndexBuffer;
        resource_buffer                UniformBuffer;
        resource_buffer                DynamicUniformBuffer;
        resource_image                 Image;
        resource_pipeline              Pipeline;
    };
};

namespace mresource
{
    void Init(frame_params FrameParams);
    
    resource_id_t Load(resource_type Type, void *Data);
};

#endif //RESOURCES_H
