#ifndef ASSET_H
#define ASSET_H

// Shader representation of a Vertex.
struct Vertex
{
    vec3 Position;
    vec3 Normals;
    vec4 Color;
    vec2 Tex0;
    
    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        return bindingDescription;
    }
    
    static JTuple<VkVertexInputAttributeDescription*, int> GetAttributeDescriptions() {
        // Create Attribute Descriptions
        VkVertexInputAttributeDescription *attribs = talloc<VkVertexInputAttributeDescription>(4);
        
        // Positions
        attribs[0].binding = 0;
        attribs[0].location = 0;
        attribs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribs[0].offset = offsetof(Vertex, Position);
        
        // Normals
        attribs[1].binding = 0;
        attribs[1].location = 1;
        attribs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribs[1].offset = offsetof(Vertex, Normals);
        
        // Color
        attribs[2].binding = 0;
        attribs[2].location = 2;
        attribs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attribs[2].offset = offsetof(Vertex, Color);
        
        // Texture
        attribs[3].binding = 0;
        attribs[3].location = 3;
        attribs[3].format = VK_FORMAT_R32G32_SFLOAT;
        attribs[3].offset = offsetof(Vertex, Tex0);
        
        JTuple<VkVertexInputAttributeDescription*, int> tuple(attribs, 4);
        return tuple;
    }
    
};

typedef i64 asset_id_t;

struct model_create_info
{
    frame_params *FrameParams;
    jstring       Filename;
};

// Mesh might have multiple primitives
struct primitive
{
    // TODO(Dustin): Attach a material here
    //MaterialParameters Material;
    //u32              MaterialId;
    
    u64              IndexCount;
    u64              VertexCount;
    
    u64              IndicesOffset;
    u64              VerticesOffset;
    
    vec3             Min;
    vec3             Max;
    
    bool             IsIndexed;
    resource_id_t    VertexBuffer;
    resource_id_t    IndexBuffer;
    
    void            *DataBlock; // Pointer to where the primitive data is organized in interleaved format
};

struct mesh
{
    ecs::Entity Entity;
    
    primitive  *Primitives;
    u64         PrimitivesCount;
    
    // TODO(Dustin): Add instances here maybe?
};

struct model_node
{
    jstring Name;
    
    model_node *Parent;
    
    model_node *Children;
    u64         ChildrenCount;
    
    bool HasTranslation;
    bool HasRotation;
    bool HasScale;
    bool HasMatrix;
    
    // NOTE(Dustin): This is an experiment. I think that
    // a node can have EITHER a Translation,Rotation,and Scale
    // OR a Model Matrix.
    
    // NOTE(Dustin): When matrix is provided, it must be decomposable
    // to TRS, and cannot skew or shear.
    
    // NOTE(Dustin): When a node is targeted for animation, only TRS
    // properties will be present; matrix will not be present.
    
    vec3 Translation;
    vec3 Scale;
    vec4 Rotation; // this is a quaternion
    
    // TODO(Dustin): Decide on whether or not to use components OR
    // matrix. If not matrix, extract the components from the matrix
    // if one was provided.
    mat4 Matrix;
    
    // Pointer to a mesh, if one exists
    mesh *Mesh;
};

struct model
{
    // a disjoint set of nodes
    model_node *Nodes;
    u32         NodesCount;
};

enum asset_type
{
    Asset_Model,
    Asset_Texture,
    Asset_Material,
};

struct asset_model
{
    model Model;
};

struct asset_texture
{
};

struct asset_material
{
};

struct asset
{
    asset_id_t Id;
    asset_type Type;
    union
    {
        asset_model    Model;
        asset_texture  Texture;
        asset_material Material;
    };
};

namespace masset
{
    asset_id_t Load(asset_type Type, void *Data);
    void Free();
    
    void Render(asset_id_t AssetId);
    
    asset* GetAsset(asset_id_t Id);
    void GetModelAssets(asset **Assets, u32 *Count);
    
    // Retrieves a list of assets of the specified type
    DynamicArray<asset*> Filter(asset_type Type);
}; // masset


#endif //ASSET_H
