#ifndef EXT_MAT_GEN_REFL_GEN_H
#define EXT_MAT_GEN_REFL_GEN_H

// TODO(Dustin): Have the shaders conform to this model
// - Global Sets are  set = 0
// - Static Sets are  set = 1 // per material set
// - Dynamic Sets are set = 2 // per material-instance set
#define GLOBAL_SET  0
#define DYNAMIC_SET 2

enum shader_type
{
    Shader_Vertex,
    Shader_Fragment,
    Shader_Geometry,
    Shader_TesselationControl,
    Shader_TesselationEvaluation,
    Shader_Compute,
};

/*

	enum BaseType
	{
		Unknown,
		Void,
		Boolean,
		SByte,
		UByte,
		Short,
		UShort,
		Int,
		UInt,
		Int64,
		UInt64,
		AtomicCounter,
		Half,
		Float,
		Double,
		Struct,
		Image,
		SampledImage,
		Sampler,
		AccelerationStructureNV,

		// Keep internal types at the end.
		ControlPointArray,
		Char
	};

*/

enum data_type
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

struct data_member
{
    data_type Type = DataType_Unknown;
    union
    {
        bool               Bool;
        i8                 Int8;
        i16                Int16;
        i32                Int32;
        i64                Int64;
        u8                 UInt8;
        u16                UInt16;
        u32                UInt32;
        u64                UInt64;
        r32                Float;
        r64                Double;
        vec2               Vec2;
        vec3               Vec3;
        vec4               Vec4;
        mat3               Mat3;
        mat4               Mat4;
        data_member_struct Struct;
    };
};

// source: https://github.com/khalladay/VkMaterialSystem/blob/master/ShaderPipeline/shaderdata.h
struct block_member
{
	u32         Size;
    u32         Offset;
    jstring     Name;
    
    data_member Member;
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
    shader_type               Type;
    
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

struct shader
{
    jstring     Filename;
    shader_type Type;
};

void GenerateReflectionInfo(DynamicArray<shader> &Shaders, jstring ReflFile);

/*

So, let's say that we know the layout of the shader ahead of time.

Can still use shader_data to determine the bind points.

Need the reflection tool to:
1. Get the data layouts of the shader
2. Create descriptor sets from the descriptor layouts

so this file is "pipeline_name.config"

**** Global, Material, Object ****
List<input_block>

Format:
Name           : u32  = ...
    Size           : u32  = ...
    Set            : u32  = ...
    Binding        : u32  = ...
    IsTextureBlock : bool = ...
    Members        : str  = [ ... ]


**** Member blocks ****

Format:
Name   : str = ...
    Size   : u32 = ...
    Offset : u32 = ...
    Member : str = [ ... ]


**** Data Block ****

Format:

Type: u32 = ...
Value: ... = ...

* Note, if the type is a struct, it's value will be an array and will have an extra member detailing the size of the array.

*/

#endif //EXT_MAT_GEN_REFL_GEN_H
