#ifndef ENGINE_GRAPHICS_RESOURCES_H
#define ENGINE_GRAPHICS_RESOURCES_H

typedef struct resource* resource_t;
struct resource_registry;

struct vertex
{
    vec3 Position;
    vec3 Normals;
    vec4 Color;
    vec2 Tex0;
};

struct simple_vertex
{
    vec3 Position;
    vec4 Color;
    vec2 Tex0;
};

enum resource_type
{
    Resource_None,
    Resource_Device,
    Resource_Swapchain,
    Resource_RenderTarget,
    Resource_Buffer,
    Resource_PipelineLayout,
    Resource_Pipeline,
    Resource_Texture2D,
};

struct resource_id
{
    u64 Index:48; // Allows for 2^48 resources
    u64 Gen:8;    // Allows for 2^8 generations before overflow
    u64 Type:7;   // Allows for 2^7 different types of of resources
    u64 Active:1; // Whether or not this resource id is active. NOTE(Dustin): Needed?
};

struct device_create_info
{
    u32      Width, Height;
    u32      RefreshRate;
    window_t Window;
    u32      SampleCount;
};

struct swapchain_create_info
{
};

struct render_target_create_info
{
    resource_id Device;
};

enum buffer_usage
{
    BufferUsage_Default,
    BufferUsage_Immutable,
    BufferUsage_Dynamic,
    BufferUsage_Staging,
};

enum buffer_cpu_access_flags
{ // TODO(Dustin): Will these ever be OR'd together?
    BufferCpuAccess_None  = BIT(0),
    BufferCpuAccess_Write = BIT(1),
    BufferCpuAccess_Read  = BIT(2),
};

// NOTE(Dustin): Add to on a as-needed basis
enum buffer_misc_flags
{
    BufferMisc_None    = BIT(0),
    BufferMisc_GenMips = BIT(1),
};

enum buffer_bind_flags
{
    BufferBind_None            = BIT(0),
    BufferBind_VertexBuffer    = BIT(1),
    BufferBind_IndexBuffer     = BIT(2),
    BufferBind_ConstantBuffer  = BIT(3),
    BufferBind_ShaderResource  = BIT(4),
    BufferBind_StreamOutput    = BIT(5),
    BufferBind_RenderTarget    = BIT(6),
    BufferBind_DepthStencil    = BIT(7),
    BufferBind_UnorderedAccess = BIT(8),
};

struct buffer_create_info
{
    resource_id             Device;
    
    u32                     Size;
    buffer_usage            Usage;
    buffer_cpu_access_flags CpuAccessFlags;
    buffer_bind_flags       BindFlags;
    buffer_misc_flags       MiscFlags;
    u32                     StructureByteStride;
    
    // Optional data
    const void             *Data;
    u32                     SysMemPitch;
    u32                     SysMemSlicePitch;
};

struct texture2d_create_info
{
    resource_id Device;
    const char *TextureFile;
};

enum pipeline_layout_format
{
    PipelineFormat_R32G32_FLOAT,
    PipelineFormat_R32G32B32_FLOAT,
    PipelineFormat_R32G32B32A32_FLOAT,
};

struct pipeline_layout_create_info
{
    const char            *Name;
    u32                    SemanticIndex; // i.e. if name is COLOR1, then semantic index is 1
    pipeline_layout_format InputFormat;
    u32                    InputSlot;
    u32                    Offset;
    bool                   PerVertex; // true if stride is per vertex
    u32                    InstanceRate;
};

struct pipeline_create_info
{
    resource_id Device;
    
    const void *VertexData;
    const void *PixelData;
    
    u32 VertexDataSize;
    u32 PixelDataSize;
    
    // TODO(Dustin): Other shader stages
    pipeline_layout_create_info *PipelineLayout;
    u32                          PipelineLayoutCount;
};

void ResourceRegistryInit(resource_registry *Registry, free_allocator *GlobalMemoryAllocator,
                          u32 MaximumResources);
void ResourceRegistryFree(resource_registry *Registry, free_allocator *GlobalMemoryAllocator);

resource_id CreateResource(resource_registry *Registry, resource_type Type, void *CreateInfo);
resource_id CreateDummyResource();
void CopyResources(resource_t *Resources, u32 *ResourcesCount, resource_registry *ResourceRegistry, tag_block_t Heap);

// A valid resource is a resource that is active and its generation matches the generation at
// the designated index.
inline bool IsValidResource(resource_t Resources, resource_id ResourceId);

#endif //RESOURCES_H
