#ifndef ENGINE_RESOURCES_SHADER_DATA_H
#define ENGINE_RESOURCES_SHADER_DATA_H

// TODO(Dustin): Have the shaders conform to this model
// - Global Sets are  set = 0
// - Static Sets are  set = 1 // per material set
// - Dynamic Sets are set = 2 // per material-instance set
#define GLOBAL_SET  0
#define STATIC_SET  1
#define DYNAMIC_SET 2

#define BASE_COLOR_TEXTURE_BINDING         0
#define METALLIC_ROUGHNESS_TEXTURE_BINDING 1
#define NORMAL_TEXTURE_BINDING             2
#define OCCLUSION_TEXTURE_BINDING          3
#define EMISSIVE_TEXTURE_BINDING           4

struct global_shader_data
{
    alignas(16) mat4 View;
    alignas(16) mat4 Projection;
};

struct object_shader_data
{
    alignas(16) mat4 Model;
};

enum shader_type
{
    Shader_Vertex,
    Shader_Fragment,
    Shader_Geometry,
    Shader_TesselationControl,
    Shader_TesselationEvaluation,
    Shader_Compute,
};

#if 0
struct input_block_serial
{
    u32                        Size;
    u32                        Set;
    u32                        Binding;
    bool                       IsTextureBlock;
    jstring                    Name;
    DynamicArray<block_member> Members;
};

struct shader_data_serial
{
    shader_type               Type;
    
    u32                       DynamicSetSize;
    u32                       StaticSetSize;
    u32                       NumDynamicUniforms;
    u32                       NumDynamicTextures;
    u32                       NumStaticUniforms;
    u32                       NumStaticTextures;
    
    input_block_serial               PushConstants;
    DynamicArray<input_block_serial> DescriptorSets;
    DynamicArray<u32>                DynamicSets;
    DynamicArray<u32>                GlobalSets;
    DynamicArray<u32>                StaticSets;
};


struct shader_descriptor_def
{
    resource_id_t          DescriptorSet;
    resource_id_t          DescriptorSetLayout;
    
    input_block           *InputData;
    u32                    InputDataCount;
    
#if 0
    // HACK(Dustin): Hard limit of 10 bindings for a descriptor set.
    // Might not need to actually store this list, but it could be useful
    // for comparing descriptor layouts...
    // NOTE(Dustin): Not going to use these right now...
    VkDescriptorSetLayoutBinding Bindings[10];
    u32                          BindingsCount;
#endif
};

#endif


enum block_data_type
{
    DataType_Unknown,
    DataType_Void,
    DataType_Boolean,
    DataType_SByte,
    DataType_UByte,
    DataType_Short,
    DataType_UShort,
    DataType_Int,
    DataType_UInt,
    DataType_Int64,
    DataType_UInt64,
    DataType_AtomicCount, // not supported as of now
    DataType_Half,
    DataType_Float,
    DataType_Double,
    DataType_Struct,
    DataType_Image,
    DataType_SampledImage,
    DataType_Sampler,
    DataType_AccelerationStructureNV,
    
    // Special types that aren't detected by spirv.
    DataType_Vec2,
    DataType_Vec3,
    DataType_Vec4,
    DataType_Mat3,
    DataType_Mat4,
};

struct data_member_struct
{
    struct block_member *Members;
    u32 MembersCount;
};

struct block_member
{
	jstring     Name;
    u32         Size;
    u32         Offset;
    
    // Base data type for the data member
    block_data_type Type = DataType_Unknown;
    // Used only if the type is DataType_Struct.
    // The actual values can be found by:
    // InputBlock.BlockMemory + BlockMember.Offset
    data_member_struct Struct;
};

struct input_block
{
    u32 Set;
    i32 Binding;
    
    // Block definition
    jstring       Name;
    bool          IsTextureBlock;
    u32           Size;
    
    block_member *Members;
    u32           MembersCount;
    
    u32           UniformOffset; // Offset into the uniform allocator. needed to bind this memory slot.
    void         *BlockMemory; // Members will write their memory here
};

struct shader_data
{
    u32                    PushConstantsCount;
    input_block           *PushConstants;
    
    u32                    GlobalInputBlockCount;
    input_block           *GlobalInputBlock;
    
    u32                    ObjectInputBlockCount;
    input_block           *ObjectInputBlock;
    
    u32                    MaterialInputBlockCount;
    input_block           *MaterialInputBlock;
    
    
#if 0
    // descriptor ids for all attached descriptor sets
    shader_descriptor_def *DescriptorSets;
    u32                    DescriptorSetsCount;
    
    // NOTE(Dustin): These might not be needed since since the
    // shader_descriptor_def list is ordered by descriptor set.
    // a list of block data is contained within each descriptor
    
    // The following are indices into Descriptor Set list
    // Useful for set bind points of certain descriptor types.
    
    // Global descriptor sets
    u32                   *GlobalSets;
    u32                    GlobalSetsCount;
    
    // Per-object descriptor data
    u32                   *ObjectSets;
    u32                    ObjectSetsCount;
    
    // Per-material descriptor data
    u32                   *MaterialSets;
    u32                    MaterialSetsCount;
    
    // TODO(Dustin): List of descriptors that have an unknown bind point...
#endif
};

#endif //ENGINE_RESOURCES_SHADER_DATA_H
