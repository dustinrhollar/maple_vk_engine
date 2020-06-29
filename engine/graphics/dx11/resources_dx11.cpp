
struct resource_device
{
    IDXGISwapChain      *Swapchain;
    ID3D11Device        *Handle;
    ID3D11DeviceContext *Context;
};

struct resource_swapchain
{
};

struct resource_render_target
{
    ID3D11RenderTargetView *Handle;
};

struct resource_pipeline_layout
{
};

struct resource_pipeline
{
    ID3D11InputLayout  *Layout;
    ID3D11VertexShader *VertexShader;
    ID3D11PixelShader  *PixelShader;
    // TODO(Dustin): The other shader types
};


struct resource_buffer
{
    ID3D11Buffer *Handle;
};

struct resource_texture
{
    ID3D11Texture2D          *Handle;
    ID3D11ShaderResourceView *View;
};

struct resource
{
    resource_id Id;
    
    union
    {
        resource_device          Device;
        resource_swapchain       Swapchain;
        resource_buffer          Buffer;
        resource_render_target   RenderTarget;
        resource_pipeline_layout PipelineLayout;
        resource_pipeline        Pipeline;
        resource_texture         Texture;
    };
};

struct resource_registry
{
    // NOTE(Dustin): It is not unresasonable to put a hard cap on resource count
    // It would remove the need for this pointer.
    //
    // Global allocator - needed for resizing
    //free_allocator *GlobalMemoryAllocator;
    
    // Allocator managing device resources
    pool_allocator  ResourceAllocator;
    
    // Resource list -  non-resizable, from global memory
    resource_t     *Resources;
    u32             ResourcesMax;
    u32             ResourcesCount;
    
    // TODO(Dustin): Unimplemented. Right now, resources are not
    // created and destroyed dynamically. This functionality is
    // here for future use, but is currently unimplemented.
    //
    // Free Indices list - resizable, from global memory
    // Requires a minimum count in order to start pulling from
    // in order to prevent re-using an index too many times
    u32            *FreeResourceIndices;
    u32             FreeResourceIndicesCap;
    u32             FreeResourceIndicesCount;
};

void ResourceRegistryInit(resource_registry *Registry, free_allocator *GlobalMemoryAllocator,
                          u32 MaximumResources)
{
    Registry->ResourcesMax   = MaximumResources;
    Registry->ResourcesCount = 0;
    
    u64 RequiredAllcoatorMemory = Registry->ResourcesMax * sizeof(resource);
    u64 RequiredResourceTMemory = Registry->ResourcesMax * sizeof(resource_t);
    
    void *AllocatorMemory = FreeListAllocatorAlloc(GlobalMemoryAllocator, RequiredAllcoatorMemory);
    PoolAllocatorInit(&Registry->ResourceAllocator, AllocatorMemory, RequiredAllcoatorMemory, sizeof(resource));
    
    // Resource list -  resizable, from global memory
    Registry->Resources = (resource_t*)FreeListAllocatorAlloc(GlobalMemoryAllocator, RequiredResourceTMemory);
}

void ResourceRegistryFree(resource_registry *Registry, free_allocator *GlobalMemoryAllocator)
{
    FreeListAllocatorAllocFree(GlobalMemoryAllocator, Registry->ResourceAllocator.Start);
    PoolAllocatorFree(&Registry->ResourceAllocator);
    
    FreeListAllocatorAllocFree(GlobalMemoryAllocator, Registry->Resources);
    Registry->Resources = NULL;
    
    Registry->ResourcesMax   = 0;
    Registry->ResourcesCount = 0;
}

resource_id CreateDummyResource()
{
    resource_id Result = {};
    Result.Type        = Resource_None;
    Result.Gen         = 0;
    Result.Active      = 0; // start off as not active
    
    return Result;
}

inline bool IsValidResource(resource_t Resources, resource_id ResourceId)
{
    return (ResourceId.Active) && (Resources[ResourceId.Index].Id.Gen == ResourceId.Gen);
}

resource_id CreateResource(resource_registry *Registry, resource_type Type, void *CreateInfo)
{
    if (Registry->ResourcesCount+1 > Registry->ResourcesMax)
    {
        mprinte("Resource max has been reached. Cannot add anymore resources!\n");
        
        resource_id Id = {};
        Id.Active = 0;
        return Id;
    }
    
    resource_id Result = {};
    Result.Type        = Type;
    Result.Gen         = 0;
    Result.Active      = 0; // start off as not active
    
    switch (Type)
    {
        case Resource_Device:
        {
            resource_device Device = {};
            
            // NOTE(Dustin): Notes on resizing...
            // IDXGISwapChain::ResizeBuffers will resize the buffer, but need to release pointers before hand
            // IDXGIFactory::MakeWindowAssociation is useful for transition between windowed and fullscreen
            //                                     with the Alt-Enter key combination.
            // More on swpachain can be found here:
            // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi
            
            // TODO(Dustin): Determine swapchain backbuffer information. Not sure what it should be set to
            // in D3D11.
            
            device_create_info *Info = (device_create_info*)CreateInfo;
            
            DXGI_SWAP_CHAIN_DESC sd;
            ZeroMemory( &sd, sizeof( sd ) );
            sd.BufferCount                        = 1;
            sd.BufferDesc.Width                   = Info->Width;
            sd.BufferDesc.Height                  = Info->Height;
            sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.BufferDesc.RefreshRate.Numerator   = Info->RefreshRate;
            sd.BufferDesc.RefreshRate.Denominator = 1;
            sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.OutputWindow                       = *reinterpret_cast<HWND*>(Info->Window);
            sd.SampleDesc.Count                   = Info->SampleCount;
            sd.SampleDesc.Quality                 = 0;
            sd.Windowed                           = TRUE;
            sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // allow full-screen switching
            
            D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
            
            UINT CreationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
            // If the project is in a debug build, enable the debug layer.
            CreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
            
            HRESULT hr = S_OK;
            D3D_FEATURE_LEVEL FeatureLevel;
            
            IDXGISwapChain      *Swapchain;
            ID3D11Device        *Handle;
            ID3D11DeviceContext *Context;
            
            hr = D3D11CreateDeviceAndSwapChain(NULL,
                                               D3D_DRIVER_TYPE_REFERENCE,
                                               NULL,
                                               CreationFlags,
                                               FeatureLevels,
                                               1,
                                               D3D11_SDK_VERSION,
                                               &sd,
                                               &Device.Swapchain,
                                               &Device.Handle,
                                               &FeatureLevel,
                                               &Device.Context);
            
            if (FAILED(hr))
            {
                mprinte("Failed to create Device and swapchain because of %d!\n", hr);
            }
            else
            {
                // Active the Id
                Result.Index  = Registry->ResourcesCount++;
                Result.Active = 1;
                
                // Create the resource
                resource *Resource = (resource*)PoolAllocatorAlloc(&Registry->ResourceAllocator);
                Resource->Id = Result;
                Resource->Device = Device;
                
                // Insert it into the list
                Registry->Resources[Resource->Id.Index] = Resource;
            }
        } break;
        
        case Resource_Swapchain:
        {
        } break;
        
        case Resource_RenderTarget:
        {
            render_target_create_info *Info = (render_target_create_info*)CreateInfo;
            
            resource_render_target RenderTarget = {};
            
            // Get the Device resources
            ID3D11Device *Device = Registry->Resources[Info->Device.Index]->Device.Handle;
            IDXGISwapChain *Swapchain = Registry->Resources[Info->Device.Index]->Device.Swapchain;
            
            // get the address of the back buffer
            ID3D11Texture2D *tBackBuffer;
            HRESULT hr = Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&tBackBuffer);
            if (FAILED(hr))
            {
                mprinte("Failed to get back buffer texture %d!\n", hr);
            }
            else
            {
                // use the back buffer address to create the render target
                hr = Device->CreateRenderTargetView(tBackBuffer, NULL, &RenderTarget.Handle);
                tBackBuffer->Release();
                if (FAILED(hr))
                {
                    mprinte("Failed to create render target view %d!\n", hr);
                }
                else
                {
                    // Active the Id
                    Result.Index  = Registry->ResourcesCount++;
                    Result.Active = 1;
                    
                    // Create the resource
                    resource *Resource = (resource*)PoolAllocatorAlloc(&Registry->ResourceAllocator);
                    Resource->Id = Result;
                    Resource->RenderTarget = RenderTarget;
                    
                    // Insert it into the list
                    Registry->Resources[Resource->Id.Index] = Resource;
                }
            }
        } break;
        
        case Resource_PipelineLayout:
        {
        } break;
        
        case Resource_Pipeline:
        {
            pipeline_create_info *Info = (pipeline_create_info*)CreateInfo;
            ID3D11Device *Device = Registry->Resources[Info->Device.Index]->Device.Handle;
            
            resource_pipeline Pipeline = {};
            
            HRESULT hr = Device->CreateVertexShader(Info->VertexData, Info->VertexDataSize, NULL, &Pipeline.VertexShader);
            if (FAILED(hr))
            {
                mprinte("Failed to create vertex shader %d!\n", hr);
                return Result;
            }
            
            hr = Device->CreatePixelShader(Info->PixelData, Info->PixelDataSize, NULL, &Pipeline.PixelShader);
            if (FAILED(hr))
            {
                mprinte("Failed to create pixel shader %d!\n", hr);
                return Result;
            }
            
            D3D11_INPUT_ELEMENT_DESC ied[16] = {};
            for (u32 i = 0; i < Info->PipelineLayoutCount; ++i)
            {
                ied[i] = {};
                ied[i].SemanticName = Info->PipelineLayout[i].Name;
                ied[i].SemanticIndex = Info->PipelineLayout[i].SemanticIndex;
                
                switch (Info->PipelineLayout[i].InputFormat)
                {
                    case PipelineFormat_R32G32_FLOAT:       ied[i].Format = DXGI_FORMAT_R32G32_FLOAT;       break;
                    case PipelineFormat_R32G32B32_FLOAT:    ied[i].Format = DXGI_FORMAT_R32G32B32_FLOAT;    break;
                    case PipelineFormat_R32G32B32A32_FLOAT: ied[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
                    default: mprinte("Format not currently supported!\n");
                }
                
                ied[i].InputSlot = Info->PipelineLayout[i].InputSlot;
                ied[i].AlignedByteOffset = Info->PipelineLayout[i].Offset;
                
                if (Info->PipelineLayout[i].PerVertex)
                    ied[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                else
                    ied[i].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                
                ied[i].InstanceDataStepRate = Info->PipelineLayout[i].InstanceRate;
            }
            
            hr = Device->CreateInputLayout(ied, Info->PipelineLayoutCount,
                                           Info->VertexData, Info->VertexDataSize,
                                           &Pipeline.Layout);
            if (FAILED(hr))
            {
                mprinte("Failed to create pipeline layout %d!\n", hr);
                return Result;
            }
            
            // Active the Id
            Result.Index  = Registry->ResourcesCount++;
            Result.Active = 1;
            
            // Create the resource
            resource *Resource = (resource*)PoolAllocatorAlloc(&Registry->ResourceAllocator);
            Resource->Id       = Result;
            Resource->Pipeline = Pipeline;
            
            // Insert it into the list
            Registry->Resources[Resource->Id.Index] = Resource;
            
        } break;
        
        
        case Resource_Buffer:
        {
            buffer_create_info *Info = (buffer_create_info*)CreateInfo;
            ID3D11Device *Device = Registry->Resources[Info->Device.Index]->Device.Handle;
            
            resource_buffer Buffer = {};
            
            D3D11_BUFFER_DESC bd;
            ZeroMemory(&bd, sizeof(bd));
            bd.ByteWidth = Info->Size;
            
            switch (Info->Usage)
            {
                case BufferUsage_Default:
                {
                    bd.Usage = D3D11_USAGE_DEFAULT; // write access access by CPU and GPU
                } break;
                
                case BufferUsage_Immutable:
                {
                    bd.Usage = D3D11_USAGE_IMMUTABLE;
                } break;
                
                case BufferUsage_Dynamic:
                {
                    bd.Usage = D3D11_USAGE_DYNAMIC;
                } break;
                
                case BufferUsage_Staging:
                {
                    bd.Usage = D3D11_USAGE_STAGING;
                } break;
                
                default: break;
            }
            
            bd.BindFlags = 0;
            if (Info->BindFlags & BufferBind_VertexBuffer)
                bd.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
            if (Info->BindFlags & BufferBind_IndexBuffer)
                bd.BindFlags |= D3D11_BIND_INDEX_BUFFER;
            if (Info->BindFlags & BufferBind_ConstantBuffer)
                bd.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
            if (Info->BindFlags & BufferBind_ShaderResource)
                bd.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
            if (Info->BindFlags & BufferBind_StreamOutput)
                bd.BindFlags |= D3D11_BIND_STREAM_OUTPUT;
            if (Info->BindFlags & BufferBind_RenderTarget)
                bd.BindFlags |= D3D11_BIND_RENDER_TARGET;
            if (Info->BindFlags & BufferBind_DepthStencil)
                bd.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
            if (Info->BindFlags & BufferBind_UnorderedAccess)
                bd.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
            
            bd.CPUAccessFlags = 0;
            if (Info->CpuAccessFlags & BufferCpuAccess_Write)
                bd.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
            if (Info->CpuAccessFlags & BufferCpuAccess_Read)
                bd.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
            
            bd.MiscFlags = 0;
            if (Info->MiscFlags & BufferMisc_GenMips)
                bd.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
            
            // Fill in the subresource data.
            HRESULT hr;
            if (Info->Data)
            {
                D3D11_SUBRESOURCE_DATA InitData = {0};
                InitData.pSysMem          = Info->Data;
                InitData.SysMemPitch      = Info->SysMemPitch;
                InitData.SysMemSlicePitch = Info->SysMemSlicePitch;
                
                hr = Device->CreateBuffer(&bd, &InitData, &Buffer.Handle);
            }
            else
            {
                hr = Device->CreateBuffer(&bd, NULL, &Buffer.Handle);
            }
            
            if (FAILED(hr))
            {
                mprinte("Failed to create vertex buffer %d!\n", hr);
                return Result;
            }
            
            
            // Active the Id
            Result.Index  = Registry->ResourcesCount++;
            Result.Active = 1;
            
            // Create the resource
            resource *Resource = (resource*)PoolAllocatorAlloc(&Registry->ResourceAllocator);
            Resource->Id       = Result;
            Resource->Buffer   = Buffer;
            
            // Insert it into the list
            Registry->Resources[Resource->Id.Index] = Resource;
            
        } break;
        
        case Resource_Texture2D:
        {
            texture2d_create_info *Info = (texture2d_create_info*)CreateInfo;
            ID3D11Device *Device = Registry->Resources[Info->Device.Index]->Device.Handle;
            
            i32 Width, Height, Channels;
            unsigned char *Data = stbi_load(Info->TextureFile, &Width, &Height, &Channels, STBI_rgb_alpha);
            u64 ImageSize = Width * Height * 4;
            
            if (!Data)
            {
                mprinte("Failed to load texture from file %s!\n", Info->TextureFile);
                return Result;
            }
            
            {
                D3D11_TEXTURE2D_DESC desc;
                memset( &desc, 0, sizeof(desc));
                
                desc.Width = 256;
                desc.Height = 256;
                desc.MipLevels = 1;
                desc.ArraySize = 1;
                desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                desc.SampleDesc.Count = 1;
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                desc.MiscFlags = 0;
                
                ID3D11Device *pd3dDevice; // Don't forget to initialize this
                ID3D11Texture2D *pTexture = NULL;
                HRESULT hr = Device->CreateTexture2D( &desc, NULL, &pTexture );
                
                
                if (FAILED(hr))
                {
                    mprinte("Failed to create exampel 2D image texture %d!\n", hr);
                    return Result;
                }
                
            }
            
            resource_texture Texture = {};
            
            D3D11_TEXTURE2D_DESC desc;
            memset( &desc, 0, sizeof(desc));
            
            // TODO(Dustin): Handle mipping
            desc.Width            = Width;
            desc.Height           = Height;
            desc.MipLevels        = 1;
            desc.ArraySize        = 1;
            desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage            = D3D11_USAGE_IMMUTABLE;
            desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags   = 0;
            desc.MiscFlags        = 0;
            
            D3D11_SUBRESOURCE_DATA InitData = {0};
            InitData.pSysMem          = Data;
            InitData.SysMemPitch      = Width * 4;
            InitData.SysMemSlicePitch = 0;
            
            HRESULT hr = Device->CreateTexture2D( &desc, &InitData, &Texture.Handle );
            
            if (FAILED(hr))
            {
                mprinte("Failed to create 2D image texture %d!\n", hr);
                return Result;
            }
            
            D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
            memset( &SRVDesc, 0, sizeof( SRVDesc ) );
            
            SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            SRVDesc.Texture2D.MipLevels = 1;
            
            hr = Device->CreateShaderResourceView(Texture.Handle, &SRVDesc, &Texture.View);
            if ( FAILED(hr) )
            {
                mprinte("Failed to create 2D image texture image view %d!\n", hr);
                
                Texture.Handle->Release();
                return Result;
            }
            
            // Active the Id
            Result.Index  = Registry->ResourcesCount++;
            Result.Active = 1;
            
            // Create the resource
            resource *Resource = (resource*)PoolAllocatorAlloc(&Registry->ResourceAllocator);
            Resource->Id       = Result;
            Resource->Texture   = Texture;
            
            // Insert it into the list
            Registry->Resources[Resource->Id.Index] = Resource;
            
        } break;
        
    };
    
    return Result;
}

void CopyResources(resource_t *Resources, u32 *ResourcesCount, resource_registry *ResourceRegistry, tag_block_t Heap)
{
    // TODO(Dustin): Account for hoels in the registry
    
    resource_t ResourcesCopy = halloc<resource>(Heap, ResourceRegistry->ResourcesCount);
    
    for (u32 i = 0; i < ResourceRegistry->ResourcesCount; ++i)
    {
        ResourcesCopy[i] = *ResourceRegistry->Resources[i];
    }
    
    *ResourcesCount = ResourceRegistry->ResourcesCount;
    *Resources = ResourcesCopy;
}
