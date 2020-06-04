#ifndef EXT_MAT_GEN_REFL_GEN_H
#define EXT_MAT_GEN_REFL_GEN_H

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

void GenerateReflectionInfo(DynamicArray<jstring> Shaders, jstring ReflFile);

#endif //EXT_MAT_GEN_REFL_GEN_H
