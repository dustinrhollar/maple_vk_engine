#ifndef ENGINE_RESOURCES_H
#define ENGINE_RESOURCES_H

typedef i64 resource_id_t;

struct dynamic_buffer_create_info;
struct global_shader_data;

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
    
    // A linear allocator that is a dynamic uniform buffer.
    // will be primarily used for Material/Instance Information
    // Allocations with be aligned to the minimum uniform offset alignment
    // TODO(Dustin): Allow for removing data from the allocator
    struct dyn_uniform_linear_alloc
    {
        u32               Alignment;
        u32               Offset;
        // one buffer per-swapchain image, when an allocation/set occurs
        // all buffers are updated.
        BufferParameters Buffer;
    };
    
    void DynUniformLinearAllocationInit(dyn_uniform_linear_alloc *Allocator, u32 TotalSize);
    void DynUniformLinearAllocationFree(dyn_uniform_linear_alloc *Allocator);
    // Returns the offset this allocation will be at in the allocator
    void DynUniformLinearAllocationAlloc(dyn_uniform_linear_alloc *Allocator, void **Data, u32 *Offset, u32 AllocationSize);
    void DynUniformLinearAllocationSetMemory(dyn_uniform_linear_alloc *Pool, u32 Offset, void *Data, u32 DataSize);
    
    
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

struct image_create_info
{
    jstring              Filename;
    
    VkFilter             MagFilter;
    VkFilter             MinFilter;
    
    VkSamplerAddressMode AddressModeU;
    VkSamplerAddressMode AddressModeV;
    VkSamplerAddressMode AddressModeW;
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

struct uniform_update_info
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
    Resource_DescriptorSetUpdate,
    Resource_VertexBuffer,
    Resource_IndexBuffer,
    Resource_UniformBuffer,
    Resource_DynamicUniformBuffer,
    Resource_DynamicBufferAllocator,
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
    u32 TextureIndex;
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

struct resource_dynamic_buffer_allocator
{
    mm::dyn_uniform_linear_alloc Allocator;
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
        
        resource_dynamic_buffer_allocator DynBufferAllocator;
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
    
    // Updates the uniform memory for the global frame data for the shaders
    void UpdateGlobalFrameData(global_shader_data *Data, u32 ImageIndex);
    
    // Updates the uniform memory for the per-object frame data for the shaders
    void UpdateObjectFrameData(struct object_shader_data *Data, u32 Offset, u32 ImageIndex);
    
    // used for DynamicUniformBuffers to reset their internal allocator
    // Do not call directly! A Uniform reset should be done through the command
    // Gpu_ResetUniformBuffer
    void ResetDynamicUniformOffset(resource_id_t Uniform, u32 ImageIdx);
    
    // Returns a template for the requested dynamic uniform. This contains data
    // for the element stride and alignment, and an offset. This is necessary
    // for binding descriptors that use a dynamic uniform buffer and for updating
    // buffer data.
    dyn_uniform_template GetDynamicUniformTemplate(resource_id_t Uniform);
    // Returns a template specifically for the global dynamic buffer...
    dyn_uniform_template GetDynamicObjectUniformTemplate();
    
    // HACK(Dustin): Get Default Resource State for materials...
    resource GetObjectDescriptorSet();
    resource GetObjectUniform();
    resource GetDefaultFrameDescriptor();
    
    // A unique struct containing the default resources in the registry.
    // A user can query for this struct in order to gain access to the default
    // resource ids
    struct default_resources
    {
        // When there are gaps in Descriptor #'s, the gaps
        // have to be filled in with an empty descriptor layout
        resource_id_t EmptyDescriptorLayout;
        
        // Default Global Descriptor, contains VP buffer
        resource_id_t DefaultGlobalDescriptor;
        resource_id_t DefaultGlobalDescriptorLayout;
        
        // Set = 1 is reserved for Model Descriptor
        // All renderable objects that is not a compute
        // material will use this descriptor.
        resource_id_t ObjectDescriptor;
        resource_id_t ObjectDescriptorLayout;
        
        resource_id_t DefaultGlobalBuffer;
        resource_id_t ObjectDynamicBuffer;
        
        // (Different from a default material)
        resource_id_t DefaultPipeline;
        
        // PBR Metallic Pipeline info
        resource_id_t MaterialDynUniform;          // Allocator for the material info
        resource_id_t PbrMaterialDescriptorLayout; // PBR Metallic Material Layout
        resource_id_t PbrMaterialDescriptor;       // PBR Metallic Material Descriptor Set
    };
    
    default_resources GetDefaultResourcesFromRegistry();
    
    // Will retrieve the next offset into the dynamic uniform buffer and update
    // the offset for the next request.
    i64 DynUniformGetNextOffset(dyn_uniform_template *DynUniformTemplate);
    
    // Allocates memory for the material from the Material Uniform Allocator.
    // Data: pointer that gets set to the memory block
    // Offset: Offset into the Uniform, needed for binding the material data
    void AllocateMaterialMemory(void **Data, u32 *Offset, u32 Size);
    
    inline bool DoesTextureArrayNeedUpdate();
    void RebindTextureArray();
};

#endif //RESOURCES_H
