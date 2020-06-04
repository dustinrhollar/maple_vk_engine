#ifndef ENGINE_RESOURCES_SHADER_DATA_H
#define ENGINE_RESOURCES_SHADER_DATA_H

// TODO(Dustin): Have the shaders conform to this model
// - Global Sets are  set = 0
// - Static Sets are  set = 1 // per material set
// - Dynamic Sets are set = 2 // per material-instance set
#define GLOBAL_SET  0
#define DYNAMIC_SET 2


// source: https://github.com/khalladay/VkMaterialSystem/blob/master/ShaderPipeline/shaderdata.h
struct block_member
{
	u32     Size;
    u32     Offset;
    jstring Name;
};

struct input_block
{
    u32                        Size;
    u32                        Set;
    u32                        Binding;
    bool                       IsTextureBlock;
    jstring                    Name;
    DynamicArray<block_member> Members;
};

struct shader_data
{
    u32                       DynamicSetSize;
    u32                       StaticSetSize;
    u32                       NumDynamicUniforms;
    u32                       NumDynamicTextures;
    u32                       NumStaticUniforms;
    u32                       NumStaticTextures;
    
    input_block               PushConstants;
    DynamicArray<input_block> DescriptorSets;
    DynamicArray<u32>         DynamicSets;
    DynamicArray<u32>         GlobalSets;
    DynamicArray<u32>         StaticSets;
};

#endif //ENGINE_RESOURCES_SHADER_DATA_H
