
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

#define VERTEX_SHADER_BIT BIT(0)
#define PIXEL_SHADER_BIT  BIT(1)

struct resource_pipeline_layout
{
    ID3D11InputLayout  *Layout;
    
    u32 ShaderBitMask;
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
    ID3D11SamplerState       *Sampler;
    
    u32 Width, Height;
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

inline bool IsValidResource(resource_id ResourceId)
{
    resource_t *Resources = GlobalResourceRegistry->Resources;
    return (ResourceId.Active) && (Resources[ResourceId.Index]->Id.Gen == ResourceId.Gen);
}

file_internal DXGI_FORMAT ConvertInputFormat(input_format Format)
{
    DXGI_FORMAT Result = DXGI_FORMAT_UNKNOWN;
    
    if (Format == InputFormat_R32_FLOAT)
        Result = DXGI_FORMAT_R32_FLOAT;
    
    else if (Format == InputFormat_R32G32_FLOAT)
        Result = DXGI_FORMAT_R32G32_FLOAT;
    
    else if (Format == InputFormat_R32G32_UINT)
        Result = DXGI_FORMAT_R32G32_UINT;
    
    else if (Format == InputFormat_R32G32B32_FLOAT)
        Result = DXGI_FORMAT_R32G32B32_FLOAT;
    
    else if (Format == InputFormat_R32G32B32A32_FLOAT)
        Result = DXGI_FORMAT_R32G32B32A32_FLOAT;
    
    else if (Format == InputFormat_R32G32B32_UINT)
        Result = DXGI_FORMAT_R32G32B32_UINT;
    
    else if (Format == InputFormat_R32G32B32A32_UINT)
        Result = DXGI_FORMAT_R32G32B32A32_UINT;
    
    return Result;
}

file_internal D3D11_USAGE ConvertBufferUsage(buffer_usage Usage)
{
    D3D11_USAGE Result = D3D11_USAGE_DEFAULT;
    
    if (Usage == BufferUsage_Default)
    {
        Result = D3D11_USAGE_DEFAULT; // write access access by CPU and GPU
    }
    
    else if (Usage == BufferUsage_Immutable)
    {
        Result = D3D11_USAGE_IMMUTABLE;
    }
    
    else if (Usage == BufferUsage_Dynamic)
    {
        Result = D3D11_USAGE_DYNAMIC;
    }
    
    else if (Usage == BufferUsage_Staging)
    {
        Result = D3D11_USAGE_STAGING;
    }
    
    return Result;
}

file_internal UINT ConvertBindFlags(buffer_bind_flags BindFlags)
{
    UINT Result = 0;
    
    if (BindFlags & BufferBind_VertexBuffer)
        Result |= D3D11_BIND_VERTEX_BUFFER;
    if (BindFlags & BufferBind_IndexBuffer)
        Result |= D3D11_BIND_INDEX_BUFFER;
    if (BindFlags & BufferBind_ConstantBuffer)
        Result |= D3D11_BIND_CONSTANT_BUFFER;
    if (BindFlags & BufferBind_ShaderResource)
        Result |= D3D11_BIND_SHADER_RESOURCE;
    if (BindFlags & BufferBind_StreamOutput)
        Result |= D3D11_BIND_STREAM_OUTPUT;
    if (BindFlags & BufferBind_RenderTarget)
        Result |= D3D11_BIND_RENDER_TARGET;
    if (BindFlags & BufferBind_DepthStencil)
        Result |= D3D11_BIND_DEPTH_STENCIL;
    if (BindFlags & BufferBind_UnorderedAccess)
        Result |= D3D11_BIND_UNORDERED_ACCESS;
    
    return Result;
}

file_internal UINT ConvertCpuAccessFlags(buffer_cpu_access_flags CpuAccessFlags)
{
    UINT Result = 0;
    
    if (CpuAccessFlags & BufferCpuAccess_Write)
        Result |= D3D11_CPU_ACCESS_WRITE;
    if (CpuAccessFlags & BufferCpuAccess_Read)
        Result |= D3D11_CPU_ACCESS_READ;
    
    return Result;
}

file_internal UINT ConvertMiscFlags(buffer_misc_flags MiscFlags)
{
    UINT Result = 0;
    
    if (MiscFlags & BufferMisc_GenMips)
        Result = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    
    return Result;
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
            ID3D11Device *Device      = GlobalRenderer->Device;
            IDXGISwapChain *Swapchain = GlobalRenderer->Swapchain;
            
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
            ID3D11Device *Device = GlobalRenderer->Device;
            
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
                ied[i].Format = ConvertInputFormat(Info->PipelineLayout[i].InputFormat);
                
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
            ID3D11Device *Device = GlobalRenderer->Device;
            
            resource_buffer Buffer = {};
            
            D3D11_BUFFER_DESC bd;
            ZeroMemory(&bd, sizeof(bd));
            bd.ByteWidth      = Info->Size;
            bd.Usage          = ConvertBufferUsage(Info->Usage);
            bd.BindFlags      = ConvertBindFlags(Info->BindFlags);
            bd.CPUAccessFlags = ConvertCpuAccessFlags(Info->CpuAccessFlags);
            bd.MiscFlags      = ConvertMiscFlags(Info->MiscFlags);
            
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
            
            ID3D11Device *Device = GlobalRenderer->Device;
            
            resource_texture Texture = {};
            
            D3D11_TEXTURE2D_DESC desc;
            memset( &desc, 0, sizeof(desc));
            
            // TODO(Dustin): Handle mipping
            desc.Width            = Info->Width;
            desc.Height           = Info->Height;
            desc.MipLevels        = 1;
            desc.ArraySize        = 1;
            desc.SampleDesc.Count = 1;
            desc.Format           = ConvertInputFormat(Info->Format);
            desc.Usage            = ConvertBufferUsage(Info->Usage);
            desc.BindFlags        = ConvertBindFlags(Info->BindFlags);
            desc.CPUAccessFlags   = ConvertCpuAccessFlags(Info->CpuAccessFlags);
            desc.MiscFlags        = ConvertMiscFlags(Info->MiscFlags);
            
            HRESULT hr;
            if (Info->Data)
            {
                D3D11_SUBRESOURCE_DATA InitData = {0};
                InitData.pSysMem          = Info->Data;
                InitData.SysMemPitch      = Info->Width * Info->Stride;
                InitData.SysMemSlicePitch = 0;
                hr = Device->CreateTexture2D( &desc, &InitData, &Texture.Handle );
            }
            else
            {
                hr = Device->CreateTexture2D( &desc, NULL, &Texture.Handle );
            }
            
            
            if (FAILED(hr))
            {
                mprinte("Failed to create 2D image texture %d!\n", hr);
                return Result;
            }
            
            D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
            memset( &SRVDesc, 0, sizeof( SRVDesc ) );
            
            SRVDesc.Format = desc.Format;
            SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            SRVDesc.Texture2D.MipLevels = 1;
            
            hr = Device->CreateShaderResourceView(Texture.Handle, &SRVDesc, &Texture.View);
            if ( FAILED(hr) )
            {
                mprinte("Failed to create 2D image texture image view %d!\n", hr);
                
                Texture.Handle->Release();
                return Result;
            }
            
            {
                D3D11_SAMPLER_DESC desc;
                ZeroMemory(&desc, sizeof(desc));
                desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
                desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
                desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
                desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
                desc.MipLODBias = 0.f;
                desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
                desc.MinLOD = 0.f;
                desc.MaxLOD = 0.f;
                hr = Device->CreateSamplerState(&desc, &Texture.Sampler);
                if ( FAILED(hr) )
                {
                    mprinte("Failed to sampler for the image %d!\n", hr);
                    
                    Texture.Handle->Release();
                    Texture.View->Release();
                    return Result;
                }
            }
            
            Texture.Width  = Info->Width;
            Texture.Height = Info->Height;
            
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

inline resource_t GetResource(resource_id Id)
{
    resource_t Result = NULL;
    
    if (!IsValidResource(Id))
    {
        mprinte("Attempted to retrieve an invalid resource!\n");
    }
    else
    {
        Result = GlobalResourceRegistry->Resources[Id.Index];
    }
    
    return Result;
}

void FreeResource(resource_id ResourceId)
{
    resource_t Resource = GetResource(ResourceId);
    
    // Invalidate the resource
    Resource->Id.Gen++;
    Resource->Id.Active = false;;
    
    switch (ResourceId.Type)
    {
        case Resource_Device:
        {
            Resource->Device.Swapchain->Release();
            Resource->Device.Handle->Release();
            Resource->Device.Context->Release();
        } break;
        
        case Resource_Swapchain:
        {
        } break;
        
        case Resource_RenderTarget:
        {
        } break;
        
        case Resource_Buffer:
        {
            Resource->Buffer.Handle->Release();
        } break;
        
        case Resource_PipelineLayout:
        {
        } break;
        
        case Resource_Pipeline:
        {
            Resource->Pipeline.Layout->Release();
            Resource->Pipeline.VertexShader->Release();
            Resource->Pipeline.PixelShader->Release();
        } break;
        
        case Resource_Texture2D:
        {
            Resource->Texture.Handle->Release();
            Resource->Texture.View->Release();
            Resource->Texture.Sampler->Release();
            
            Resource->Texture.Width  = 0;
            Resource->Texture.Height = 0;
        } break;
    }
}

void CopyResources(resource_t *Resources, u32 *ResourcesCount, resource_registry *ResourceRegistry, tag_block_t Heap)
{
    // TODO(Dustin): Account for holes in the registry
    
    resource_t ResourcesCopy = halloc<resource>(Heap, ResourceRegistry->ResourcesCount);
    
    for (u32 i = 0; i < ResourceRegistry->ResourcesCount; ++i)
    {
        ResourcesCopy[i] = *ResourceRegistry->Resources[i];
    }
    
    *ResourcesCount = ResourceRegistry->ResourcesCount;
    *Resources = ResourcesCopy;
}

