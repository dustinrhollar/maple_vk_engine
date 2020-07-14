#ifndef ENGINE_ASSETS_ASSET_H
#define ENGINE_ASSETS_ASSET_H

struct resource_id;
struct resource_registry;
typedef struct asset* asset_t;


struct asset_id
{
    u64 Index:48; // Allows for 2^48 assets
    u64 Gen:8;    // Allows for 2^8 generations before overflow
    u64 Type:7;   // Allows for 2^7 different types of of assets
    u64 Active:1; // Whether or not this asset id is active. NOTE(Dustin): Needed?
};

enum asset_type
{
    Asset_SimpleModel,
    Asset_Model,
    Asset_Texture,
    Asset_Material,
};

struct simple_model_create_info
{
    resource_registry *ResourceRegistry;
    
    void              *Vertices;
    u32                VerticesCount;
    u32                VertexStride;
    
    void              *Indices;
    u32                IndicesCount;
    u32                IndicesStride;
    
    const char        *VertexShader;
    const char        *PixelShader;
    
    const char        *DiffuseTextureFilename;
    
    // TODO(Dustin): Other material info?
};

struct asset_material
{
    
};

struct asset_texture
{
    
};

//~ Model related objects
struct primitive
{
    asset_id         Material;
    
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
    
    resource_id      VertexBuffer;
    resource_id      IndexBuffer;
};

struct mesh
{
    mstring     Name; // Keep this?
    
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
    mstring      Name;
    
    model_node  *Parent;
    
    model_node **Children;
    u64          ChildrenCount;
    
    vec3         Translation;
    vec3         Scale;
    vec4         Rotation; // this is a quaternion
    
    // Pointer to a mesh, if one exists
    mesh        *Mesh;
    
    mstring      MeshName;
};

struct asset_model
{
    model_node **RootModelNodes;
    i32          RootModelNodesCount;
    
    model_node  *Nodes;
    i32          NodesCount;
    
    mesh        *Meshes;
    i32          MeshesCount;
    
    primitive   *Primitives;
    i32          PrimitivesCount;
};

// NOTE(Dustin): Temporary asset used for early testing
struct asset_simple_model
{
    resource_id VertexBuffer;
    resource_id IndexBuffer;
    
    u32         VertexCount;
    u32         VertexStride;
    u32         VertexOffset; // offset into a buffer?
    
    resource_id Pipeline;
    
    resource_id DiffuseTexture;
};


struct asset
{
    asset_id Id;
    union
    {
        asset_simple_model SimpleModel;
        asset_model        Model;
    };
};

struct asset_registry
{
    // the asset registry does not *own* this pointer.
    renderer_t      Renderer;
    
    // Allocator managing device resources
    pool_allocator  AssetAllocator;
    
    // Resource list -  non-resizable, from global memory
    asset_t        *Assets;
    u32             AssetsMax;
    u32             AssetsCount;
    
    // TODO(Dustin): Free Indices
    // See note left in the resource_registry
};


void AssetRegistryInit(asset_registry *Registry, renderer_t Renderer, free_allocator *GlobalMemoryAllocator,
                       u32 MaximumAssets);
void AssetRegistryFree(asset_registry *Registry, free_allocator *GlobalMemoryAllocator);

asset_id CreateAsset(asset_registry *Registry, asset_type Type, void *CreateInfo);
void CopyAssets(asset_t *Assets, u32 *AssetsCount, asset_registry *AssetRegistry, tag_block_t Heap);

inline bool IsValidAsset(asset_t Assets, asset_id Id);

#endif //ENGINE_ASSETS_ASSET_H
