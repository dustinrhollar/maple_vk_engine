//asset.cpp

void AssetRegistryInit(asset_registry *Registry, renderer_t Renderer, free_allocator *GlobalMemoryAllocator,
                       u32 MaximumAssets)
{
    Registry->Renderer    = Renderer;
    Registry->AssetsMax   = MaximumAssets;
    Registry->AssetsCount = 0;
    
    u64 RequiredAllocatorMemory = Registry->AssetsMax * sizeof(asset);
    u64 RequiredResourceTMemory = Registry->AssetsMax * sizeof(asset_t);
    
    void *AllocatorMemory = FreeListAllocatorAlloc(GlobalMemoryAllocator, RequiredAllocatorMemory);
    PoolAllocatorInit(&Registry->AssetAllocator, AllocatorMemory, RequiredAllocatorMemory, sizeof(asset));
    
    // Resource list -  resizable, from global memory
    Registry->Assets = (asset_t*)FreeListAllocatorAlloc(GlobalMemoryAllocator, RequiredResourceTMemory);
}

void AssetRegistryFree(asset_registry *Registry, free_allocator *GlobalMemoryAllocator)
{
    FreeListAllocatorAllocFree(GlobalMemoryAllocator, Registry->AssetAllocator.Start);
    PoolAllocatorFree(&Registry->AssetAllocator);
    
    FreeListAllocatorAllocFree(GlobalMemoryAllocator, Registry->Assets);
    Registry->Assets = NULL;
    
    Registry->AssetsMax   = 0;
    Registry->AssetsCount = 0;
    Registry->Renderer    = NULL;
}


asset_id CreateAsset(asset_registry *Registry, asset_type Type, void *CreateInfo)
{
    asset_id Result = {};
    Result.Type        = Type;
    Result.Gen         = 0;
    Result.Active      = 0; // start off as not active
    
    if (Registry->AssetsCount+1 > Registry->AssetsMax)
    {
        mprinte("asset max has been reached. Cannot add anymore assets!\n");
        return Result;
    }
    
    switch (Type)
    {
        case Asset_SimpleModel:
        {
            simple_model_create_info *Info = (simple_model_create_info*)CreateInfo;
            
            asset_simple_model SimpleModel = {};
            SimpleModel.VertexCount  = Info->VerticesCount;
            SimpleModel.VertexStride = Info->VertexStride;
            SimpleModel.VertexOffset = 0;
            
            // NOTE(Dustin): For now, hardcode the layout info
            pipeline_layout_create_info LayoutInfo[3] = {
                {"POSITION",  0, PipelineFormat_R32G32B32_FLOAT,    0, 0,  true, 0},
                {"COLOR",     0, PipelineFormat_R32G32B32A32_FLOAT, 0, 12, true, 0},
                {"TEXCOORD",  0, PipelineFormat_R32G32_FLOAT,       0, 28, true, 0},
            };
            u32 LayoutCount = sizeof(LayoutInfo) / sizeof(LayoutInfo[0]);
            
            file_t VertFile = PlatformLoadFile(Info->VertexShader);
            file_t FragFile = PlatformLoadFile(Info->PixelShader);
            
            pipeline_create_info PipelineInfo = {};
            PipelineInfo.Device              = Registry->Renderer->Device;
            PipelineInfo.VertexData          = GetFileBuffer(VertFile);
            PipelineInfo.PixelData           = GetFileBuffer(FragFile);
            PipelineInfo.VertexDataSize      = PlatformGetFileSize(VertFile);
            PipelineInfo.PixelDataSize       = PlatformGetFileSize(FragFile);
            PipelineInfo.PipelineLayout      = LayoutInfo;
            PipelineInfo.PipelineLayoutCount = LayoutCount;
            SimpleModel.Pipeline = CreateResource(Info->ResourceRegistry, Resource_Pipeline, &PipelineInfo);
            
            buffer_create_info BufferInfo = {};
            BufferInfo.Device              = Registry->Renderer->Device;
            BufferInfo.Size                = Info->VertexStride * Info->VerticesCount;
            BufferInfo.Usage               = BufferUsage_Default;
            BufferInfo.CpuAccessFlags      = BufferCpuAccess_None;
            BufferInfo.BindFlags           = BufferBind_VertexBuffer;
            BufferInfo.MiscFlags           = BufferMisc_None;
            BufferInfo.StructureByteStride = 0;
            BufferInfo.Data                = Info->Vertices;
            BufferInfo.SysMemPitch         = 0;
            BufferInfo.SysMemSlicePitch    = 0;
            SimpleModel.VertexBuffer = CreateResource(Info->ResourceRegistry, Resource_Buffer, &BufferInfo);
            
            // TODO(Dustin): Indices
            SimpleModel.IndexBuffer = CreateDummyResource();
            
            if (Info->DiffuseTextureFilename)
            {
                texture2d_create_info CreateInfo = {};
                CreateInfo.Device = Registry->Renderer->Device;
                CreateInfo.TextureFile = Info->DiffuseTextureFilename;
                
                SimpleModel.DiffuseTexture = CreateResource(Info->ResourceRegistry, Resource_Texture2D, &CreateInfo);
            }
            else
            {
                SimpleModel.DiffuseTexture = CreateDummyResource();
            }
            
            // Active the Id
            Result.Index  = Registry->AssetsCount++;
            Result.Active = 1;
            
            // Create the resource
            asset *Asset       = (asset*)PoolAllocatorAlloc(&Registry->AssetAllocator);
            Asset->Id          = Result;
            Asset->SimpleModel = SimpleModel;
            
            // Insert it into the list
            Registry->Assets[Asset->Id.Index] = Asset;
            
        } break;
        
        case Asset_Model:
        {
        } break;
        
        case Asset_Texture:
        {
        } break;
        
        case Asset_Material:
        {
        } break;
        
        default: mprinte("Unknown asset type!\n"); break;
    };
    
    return Result;
}

void CopyAssets(asset_t *Assets, u32 *AssetsCount, asset_registry *AssetRegistry, tag_block_t Heap)
{
    // TODO(Dustin): Account for hoels in the registry
    
    asset_t AssetsCopy = halloc<asset>(Heap, AssetRegistry->AssetsCount);
    
    for (u32 i = 0; i < AssetRegistry->AssetsCount; ++i)
    {
        AssetsCopy[i] = *AssetRegistry->Assets[i];
    }
    
    *AssetsCount = AssetRegistry->AssetsCount;
    *Assets = AssetsCopy;
}

inline bool IsValidAsset(asset_t Assets, asset_id Id)
{
    return Id.Active && (Assets[Id.Index].Id.Gen == Id.Gen);
}
