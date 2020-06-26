#ifndef ENGINE_ASSETS_GLTF_CONVERTER_H
#define ENGINE_ASSETS_GLTF_CONVERTER_H


/*
-- Format of the internal model file:
str       -> Name of the binary file
i32       -> Number of primitives
Primitive List
---- u64  -> base Offset into the Data Block
 ---- u32  -> Number of Indices
---- u32  -> Index Stride
---- u32  -> Number of Vertices
---- u32  -> Vertex Stride
---- bool -> whether or not it is a skinned vertex
---- vec3 -> Minimum vertex position
 ---- vec3 -> Maximum vertex position
 i32       -> Number of meshes
Mesh List
---- str  -> Mesh Name
---- u64  -> Number of Primitives
---- Primitive Idx List
-------- i32 -> Index into the primitive array
i32       -> Number of nodes
Node List
---- str  -> Name of the node
---- i32  -> Index of the parent node
---- u64  -> Number of Children the node has
---- Children Index List
-------- i32 -> Index into the Node array
---- vec3 -> vector for translation
---- vec3 -> vector for scale
---- vec4 -> vector for rotation
---- i32  -> idx of the mesh. If one is not attached, then a -1 has been written
i32       -> Number of Scenes in the file
Scene List
---- Node index List
-------- i32 -> Index of the root node in the node list
*/


/*
From the GLTF 2.0 Specifications:
The alphaMode property defines how the alpha value of the main factor and texture should be
interpreted. The alpha value is defined in the baseColor for metallic-roughness material model.
alphaMode can be one of the following values:
    OPAQUE - The rendered output is fully opaque and any alpha value is ignored.
    MASK   - The rendered output is either fully opaque or fully transparent depending on the alpha
             value and the specified alpha cutoff value. This mode is used to simulate geometry such
             as tree leaves or wire fences.
    BLEND  - The rendered output is combined with the background using the normal painting operation
             (i.e. the Porter and Duff over operator). This mode is used to simulate geometry such
             as guaze cloth or animal fur.
When alphaMode is set to MASK the alphaCutoff property specifies the cutoff threshold. If the alpha
value is greater than or equal to the alphaCutoff value then it is rendered as fully opaque,
otherwise, it is rendered as fully transparent. alphaCutoff value is ignored for other modes.
Implementation Note:
OPAQUE - A depth value is written for every pixel and mesh sorting is not required for correct output.
MASK   - A depth value is not written for a pixel that is discarded after the alpha test. A depth
         value is written for all other pixels. Mesh sorting is not required for correct output.
BLEND  - Support for this mode varies. There is no perfect and fast solution that works for all cases.
         Implementations should try to achieve the correct blending output for as many situations as
         possible. Whether depth value is written or whether to sort is up to the implementation.
         For example, implementations can discard pixels which have zero or close to zero alpha
         value to avoid sorting issues.
*/
enum TextureAlphaMode
{
    TEXTURE_ALPHA_MODE_OPAQUE,
    TEXTURE_ALPHA_MODE_MASK,
    TEXTURE_ALPHA_MODE_BLEND,
    TEXTURE_ALPHA_MODE_INVALID,
};

struct texture_serial
{
    mstring Filename;
    
    i32 MagFilter;
	i32 MinFilter;
	
    i32 AddressModeU;
    i32 AddressModeV;
    i32 AddressModeW;
    
    // TODO(Dustin): Transforms for a texture?
    //r32 Scale;
    //bool HasTransform;
};

struct material_serial
{
    // NOTE(Dustin): Name stored in a SystemInfo.
    // It is redundant to store the name here.
    mstring Name;
    
    // NOTE(Dustin): For now, I am going to assumed that only
    // one of these will ever be true - that way each pipeline's
    // settings can be placed in a union to reduce struct size/
    
    // TODO(Dustin): Assuming that only one of these can be true,
    // turn the booleans into an enumeration, or even make different
    // meterial parameter types.
    bool HasPBRMetallicRoughness;
    bool HasPBRSpecularGlossiness;
    bool HasClearCoat;
    
    union {
        // Metallic - Roughness Pipeline
        struct
        {
            texture_serial BaseColorTexture;
            texture_serial MetallicRoughnessTexture;
            
            vec4           BaseColorFactor; // Will this always be a vec4?
            r32            MetallicFactor;
            r32            RoughnessFactor;
        };
        
        // Specilar - Glosiness Pipeline
        struct
        {
            texture_serial DiffuseTexture;
            texture_serial SpecularGlossinessTexture;
            
            vec4           DiffuseFactor;
            vec3           SpecularFactor;
            r32            GlossinessFactor;
        };
        
        // ClearCoat Pipeline
        struct
        {
            texture_serial ClearCoatTexture;
            texture_serial ClearCoatRoughnessTexture;
            texture_serial ClearCoatNormalTexture;
            
            r32            ClearCoatFactor;
            r32            ClearCoatRoughnessFactor;
        };
    };
    
    texture_serial NormalTexture;
    texture_serial OcclusionTexture;
    texture_serial EmissiveTexture;
    
    // Alpha mode?
    TextureAlphaMode AlphaMode;
    r32              AlphaCutoff;
    
    bool DoubleSided;
    bool Unlit;
};

struct primitive_serial
{
    i32 PrimitiveIdx;
    
    mstring MaterialFile; // NOTE(Dustin): Why did I need the filename?
    mstring MaterialName;
    
    u64     Offset; // offset into the binary file
    
    bool    IsIndexed;
    u32     IndexCount;
    u32     VertexCount;
    bool    IsSkinned; // whether or not the primitive is skinned
    
    vec3    Min;
    vec3    Max;
    
    u32     VertexStride; // generally...
    u32     IndexStride;  // generally sizeof(u32)
};

struct mesh_serial
{
    mstring MeshName;
    i32     MeshIdx;
    
    i32    *PrimitivesIdx;
    u64     PrimitivesCount;
};

struct model_node_serial
{
    mstring Name;
    mstring MeshName;
    
    i32 NodeIdx;
    i32 ParentIdx = -1; // index into a node array
    
    
    i32 *ChildrenIndices;
    u64  ChildrenCount;
    
    // NOTE(Dustin): This is an experiment. I think that
    // a node can have EITHER a Translation,Rotation,and Scale
    // OR a Model Matrix.
    
    // NOTE(Dustin): When matrix is provided, it must be decomposable
    // to TRS, and cannot skew or shear.
    
    // NOTE(Dustin): When a node is targeted for animation, only TRS
    // properties will be present; matrix will not be present.
    
    bool HasTranslation;
    bool HasRotation;
    bool HasScale;
    bool HasMatrix;
    
    vec3 Translation;
    vec3 Scale;
    vec4 Rotation; // this is a quaternion
    
    // NOTE(Dustin): if given a tranformation matrix, extract the components...
    mat4 Matrix;
    
    // Pointer to a mesh, if one exists
    i32 MeshIdx;
};

struct scene_serial
{
    i32 SceneIdx;
    
    // a disjoint set of nodes
    i32               *NodesIdx;
    u32                NodesCount;
};

struct mesh_converter
{
    cgltf_data        *Data;
    mstring            Filename;
    mstring            Directory;
    
    scene_serial      *SerialSceneList;
    i32                SceneIdx;
    
    model_node_serial *SerialNodeList;
    i32                NodeIdx;
    
    mesh_serial       *SerialMeshList;
    i32                MeshIdx;
    
    primitive_serial  *SerialPrimitiveList;
    i32                PrimitiveIdx;
    
    mstring           *MaterialNameList;
    i32                MaterialNameListIdx;
    
    // the binary block for the primitive data information
    file_t             BinaryDataFile;
    u64                CurrentBinaryFileOffset;
    
    // Allocator for temporary memory
    tag_block_t        Heap;
};

void ConvertGltfMesh(tag_block_t Heap, const char *Filename);

#endif //ENGINE_ASSETS_GLTF_CONVERTER_H
