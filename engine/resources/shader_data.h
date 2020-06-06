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
struct block_member_serial
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
    DynamicArray<block_member_serial> Members;
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

struct shader_data
{
};

/*

Descriptor Set Layout Information Required
- Binding
- Descriptor count
- Type (Buffer/Dynamic/...)
- Stage
- Immutable Samplers

Descriptor Set Information
- DescriptorSetLayout

Material Information
- List of shader files
- Descriptor Layouts
- Shader bindings

For now, let's only consider two buffers: VP (Global) and Model (Object).
Once I get textures in place, then add in samplers and see what needs to change.
This also means that descriptors and materials need to get hidden from the game
layer.

VkDescriptorSetLayoutBinding Bindings[1]= {};
    Bindings[0].binding            = 0;
    Bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    Bindings[0].descriptorCount    = 1;
    Bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    Bindings[0].pImmutableSamplers = nullptr; // Optional
    
here is a good example

*/

#endif //ENGINE_RESOURCES_SHADER_DATA_H
