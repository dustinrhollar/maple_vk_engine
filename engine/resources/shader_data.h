#ifndef ENGINE_RESOURCES_SHADER_DATA_H
#define ENGINE_RESOURCES_SHADER_DATA_H

// TODO(Dustin): Have the shaders conform to this model
// - Global Sets are  set = 0
// - Static Sets are  set = 1 // per material set
// - Dynamic Sets are set = 2 // per material-instance set
#define GLOBAL_SET  0
#define STATIC_SET  1
#define DYNAMIC_SET 2

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

// source: https://github.com/khalladay/VkMaterialSystem/blob/master/ShaderPipeline/shaderdata.h
struct block_member
{
	u32     Size;
    u32     Offset;
    jstring Name;
};

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

// TODO(Dustin): Determine if it possible to have duplicated bind points.
// For example, can bind points at set=0 binding=0 be used for Model in the vertex
// and color in the fragment?
struct input_block
{
#if 0
    // NOTE(Dustin):
    // Set and binding can be found by their position
    // in external arrays...
    // shader_descriptor_def will be the set position in the
    // material data struct.
    // input_block will be the binding position in the shader
    // descriptor struct.
    u32           Set;
    u32           Binding;
#endif
    
    // Block definition
    jstring       Name;
    bool          IsTextureBlock;
    u32           Size;
    
    block_member *Members;
    u32           MembersCount;
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

struct shader_data
{
    input_block            PushConstants;
    
    // descriptor ids for all attached descriptor sets
    shader_descriptor_def *DescriptorSets;
    u32                    DescriptorSetsCount;
    
#if 0
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
