#ifndef MESH_CONVERTER_H
#define MESH_CONVERTER_H

/*

-- Format of the internal model file:

u64       -> Size of the Data Block (indices, vertices)
char list -> Data Block
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
---- str  -> Name of the mesh. If one is not attached, then a single u32 with value of 0 has been written
---- i32  -> Index of the parent node
---- u64  -> Number of Children the node has
---- Children Index List
-------- i32 -> Index into the Node array
---- vec3 -> vector for translation
---- vec3 -> vector for scale
---- vec4 -> vector for rotation

*/


struct primitive_serial
{
    i32 PrimitiveIdx;
    
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
    jstring MeshName;
    i32     MeshIdx;
    
    i32    *PrimitivesIdx;
    u64     PrimitivesCount;
};

struct model_node_serial
{
    jstring Name;
    jstring MeshName;
    
    i32 NodeIdx;
    i32 ParentIdx = -1; // index into a node array
    
    
    i32 *ChildrenIndices;
    u64  ChildrenCount;
    
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

struct model_serial
{
    i32 ModelIdx;
    
    // a disjoint set of nodes
    i32               *NodesIdx;
    u32                NodesCount;
};


struct mesh_converter
{
    cgltf_data        *Data;
    jstring            Filename;
    jstring            Directory;
    
    model_serial      *SerialModelList;
    i32                ModelIdx;
    
    model_node_serial *SerialNodeList;
    i32                NodeIdx;
    
    mesh_serial       *SerialMeshList;
    i32                MeshIdx;
    
    primitive_serial  *SerialPrimitiveList;
    i32                PrimitiveIdx;
    
    // the binary block for the primitive data information
    FileBuffer         PrimitiveDataBlock;
};


namespace masset
{
    void ConvertGlTF(jstring Filename);
}; // masset


#endif //MESH_CONVERTER_H
