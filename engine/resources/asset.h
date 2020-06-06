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

enum asset_type
{
    Asset_Invalid    = BIT(0),
    Asset_Model      = BIT(1),
    Asset_Texture    = BIT(2),
    Asset_Material   = BIT(3),
};

struct asset_id
{
    u64 Type:8;
    u64 Gen:16;
    u64 Index:40;
};

typedef asset_id asset_id_t;

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
    
    u32              IndexCount;
    u32              VertexCount;
    
    u32              IndexStride;
    u32              VertexStride;
    
    u64              IndicesOffset;
    u64              VerticesOffset;
    
    vec3             Min;
    vec3             Max;
    
    bool             IsIndexed;
    bool             IsSkinned;
    
    resource_id_t    VertexBuffer;
    resource_id_t    IndexBuffer;
};

struct mesh
{
    jstring     Name; // Keep this?
    
    primitive **Primitives;
    u64         PrimitivesCount;
    
    // TODO(Dustin): Add instances here maybe?
};

/*

Three known types of nodes:
- Transformation Node : CHECK
- Joint Node          : DONT CARE
- Mesh Node           : CHECK

*/

struct model_node
{
    jstring     Name;
    
    model_node *Parent;
    
    model_node **Children;
    u64         ChildrenCount;
    
    vec3        Translation;
    vec3        Scale;
    vec4        Rotation; // this is a quaternion
    
    // Pointer to a mesh, if one exists
    mesh       *Mesh;
    
    jstring     MeshName;
};

struct asset_model
{
    ecs::Entity Entity;
    
    model_node **RootModelNodes;
    i32          RootModelNodesCount;
    
    model_node  *Nodes;
    i32          NodesCount;
    
    mesh        *Meshes;
    i32          MeshesCount;
    
    primitive   *Primitives;
    i32          PrimitivesCount;
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
    asset_type Type; // TODO(Dustin): Remove this. It is encoded in the Id.
    union
    {
        asset_model    Model;
        asset_texture  Texture;
        asset_material Material;
    };
};

namespace masset
{
    void Init();
    void Free();
    
    asset_id_t Load(asset_type Type, void *Data);
    
    void Render(asset_id_t AssetId);
    
    asset* GetAsset(asset_id_t Id);
    void GetAssetList(asset **Assets, u32 *Count);
    void FilterAssets(asset **Assets, u32 *Count, asset *AssetList, u32 AssetListCount, asset_type Type);
    
    // Retrieves a list of assets of the specified type
    DynamicArray<asset*> Filter(asset_type Type);
}; // masset


#endif //ASSET_H
