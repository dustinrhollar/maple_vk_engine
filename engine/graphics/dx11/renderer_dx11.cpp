
struct framebuffer
{
    ID3D11Device           *Device;
    ID3D11DeviceContext    *Context;
    
    ID3D11Texture2D        *RenderTargetTexture;
    ID3D11RenderTargetView *RenderTarget;
    
    u32                     Width;
    u32                     Height;
};

void FramebufferInit(framebuffer            *Framebuffer, 
                     ID3D11Device           *Device,
                     ID3D11DeviceContext    *Context,
                     u32 Width, u32 Height)
{
    Framebuffer->Device = Device;
    Framebuffer->Context = Context;
    
    // Create Texture2D for the frame buffer
    D3D11_TEXTURE2D_DESC desc;
    memset( &desc, 0, sizeof(desc));
    
    desc.Width            = Width;
    desc.Height           = Height;
    desc.MipLevels        = 1;
    desc.ArraySize        = 1;
    desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_DEFAULT;
    desc.BindFlags        = D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags   = 0;
    desc.MiscFlags        = 0;
    
    HRESULT hr = Device->CreateTexture2D(&desc, NULL, &Framebuffer->RenderTargetTexture);
    if (FAILED(hr))
    {
        mprinte("Failed the frame buffer render target texture %d!\n", hr);
        
        Framebuffer->Device = NULL;
        Framebuffer->Context = NULL;
        
        return;
    }
    
    // Create Render Target using the new Texture
    hr = Device->CreateRenderTargetView(Framebuffer->RenderTargetTexture, NULL, &Framebuffer->RenderTarget);
    if (FAILED(hr))
    {
        mprinte("Failed the frame buffer render target %d!\n", hr);
        
        Framebuffer->Device = NULL;
        Framebuffer->Context = NULL;
        
        return;
    }
    
    Framebuffer->Width = Width;
    Framebuffer->Height = Height;
}

void FramebufferFree(framebuffer *Framebuffer)
{
}

void FramebufferResize(framebuffer *Framebuffer, u32 Width, u32 Height)
{
}

void RendererInit(free_allocator *Allocator, resource_registry *Registry,
                  window_t Window, u32 Width, u32 Height, u32 RefreshRate)
{
    GlobalRenderer = palloc<renderer>(Allocator);
    
    //~ Create the device
    
    // NOTE(Dustin): Notes on resizing...
    // IDXGISwapChain::ResizeBuffers will resize the buffer, but need to release pointers before hand
    // IDXGIFactory::MakeWindowAssociation is useful for transition between windowed and fullscreen
    //                                     with the Alt-Enter key combination.
    // More on swpachain can be found here:
    // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi
    
    // TODO(Dustin): Determine swapchain backbuffer information. Not sure what it should be set to
    // in D3D11.
    
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount                        = 1;
    sd.BufferDesc.Width                   = Width;
    sd.BufferDesc.Height                  = Height;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = RefreshRate;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = *reinterpret_cast<HWND*>(Window);
    sd.SampleDesc.Count                   = 1;
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
    
    hr = D3D11CreateDeviceAndSwapChain(NULL,
                                       D3D_DRIVER_TYPE_REFERENCE,
                                       NULL,
                                       CreationFlags,
                                       FeatureLevels,
                                       1,
                                       D3D11_SDK_VERSION,
                                       &sd,
                                       &GlobalRenderer->Swapchain,
                                       &GlobalRenderer->Device,
                                       &FeatureLevel,
                                       &GlobalRenderer->DeviceContext);
    
    if (FAILED(hr))
    {
        mprinte("Failed to create Device and swapchain because of %d!\n", hr);
    }
    
    //~ Create the swapchain render target
    
    ID3D11Texture2D *tBackBuffer;
    hr = GlobalRenderer->Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&tBackBuffer);
    if (FAILED(hr))
    {
        PlatformFatalError("Failed to get back buffer texture when creating the swapchain render target %d!\n", hr);
    }
    
    // use the back buffer address to create the render target
    hr = GlobalRenderer->Device->CreateRenderTargetView(tBackBuffer, NULL, &GlobalRenderer->RenderTarget);
    tBackBuffer->Release();
    if (FAILED(hr))
    {
        PlatformFatalError("Failed to create render target view for the swapchain %d!\n", hr);
    }
    
    //~ Create the main Maple Gui Window
    
    GlobalRenderer->WindowStack = {};
    GlobalRenderer->WindowStack.WindowCount = 1;
    
    maple_ui *MainUi = palloc<maple_ui>(Allocator);
    *MainUi = {};
    
    // Setup terrain defaults
    {
        terrain_settings TerrainSettings = {};
        TerrainSettings.TerrainMeshUpdated    = false;
        TerrainSettings.HeightmapUpdated      = false;
        
        TerrainSettings.TerrainWidth  = 256;
        TerrainSettings.TerrainHeight = 256;
        
        // Simple Sim. Settings
        TerrainSettings.HeightmapWidth  = 512;
        TerrainSettings.HeightmapHeight = 512;
        TerrainSettings.NumberOfOctaves = 8;
        TerrainSettings.Persistence     = 0.5f;
        TerrainSettings.Low             = 0.00f;
        TerrainSettings.High            = 1.0f;
        TerrainSettings.Exp             = 2.00f;
        
        // Thermal Erosion Settings
        TerrainSettings.ThermalEnabled       = false;
        TerrainSettings.ThermalNumIterations = 20;
        
        // Inverse Thermal Settings
        TerrainSettings.InverseThermalEnabled       = false;
        TerrainSettings.InverseThermalNumIterations = 20;
        
        // Hydraulic Settings
        TerrainSettings.HydraulicEnabled               = false;
        TerrainSettings.HydraulicNumIterations         = 20;
        TerrainSettings.RainConstant                   = 0.01f;
        TerrainSettings.SolubilityConstant             = 0.01;
        TerrainSettings.EvaporationCoefficient         = 0.5f;
        TerrainSettings.SedimentTransferMaxCoefficient = 0.01f;
        
        MainUi->TerrainSettings = TerrainSettings;
    }
    
    GlobalRenderer->WindowStack.Stack[MAPLE_MAIN_UI_INDEX].Callback     = &EngineMainUi;
    GlobalRenderer->WindowStack.Stack[MAPLE_MAIN_UI_INDEX].FreeCallback = &EngineMainUiFree;
    GlobalRenderer->WindowStack.Stack[MAPLE_MAIN_UI_INDEX].Data         = MainUi;
}

void RendererShutdown(free_allocator *Allocator)
{
    // Need a seperation of of the MAIN UI because it is possible the 
    // main ui has been closed, but other windows are open
    if (GlobalRenderer->WindowStack.Stack[MAPLE_MAIN_UI_INDEX].Data)
    {
        GlobalRenderer->WindowStack.Stack[MAPLE_MAIN_UI_INDEX]
            .FreeCallback(GlobalRenderer->WindowStack.Stack[MAPLE_MAIN_UI_INDEX].Data);
    }
    FreeListAllocatorAllocFree(Allocator, GlobalRenderer->WindowStack.Stack[MAPLE_MAIN_UI_INDEX].Data);
    
    for (u32 i = 1; i < GlobalRenderer->WindowStack.WindowCount; ++i)
    {
        GlobalRenderer->WindowStack.Stack[i].FreeCallback(GlobalRenderer->WindowStack.Stack[i].Data);
    }
    
    pfree<renderer>(Allocator, GlobalRenderer);
    GlobalRenderer = NULL;
}

void RendererResize(resource_registry *Registry)
{
#if 0
    ID3D11Device *Device = Registry->Resources[Renderer->Device.Index]->Device.Handle;
    ID3D11DeviceContext *DeviceContext = Registry->Resources[Renderer->Device.Index]->Device.Context;
    IDXGISwapChain *Swapchain = Registry->Resources[Renderer->Device.Index]->Device.Swapchain;
    ID3D11RenderTargetView **RenderTarget = &Registry->Resources[Renderer->RenderTarget.Index]->RenderTarget.Handle;
#else
    
    ID3D11Device *Device                  = GlobalRenderer->Device;
    ID3D11DeviceContext *DeviceContext    = GlobalRenderer->DeviceContext;
    IDXGISwapChain *Swapchain             = GlobalRenderer->Swapchain;
    ID3D11RenderTargetView **RenderTarget = &GlobalRenderer->RenderTarget;
    
#endif
    
    DeviceContext->OMSetRenderTargets(0, 0, 0);
    (*RenderTarget)->Release();
    
    HRESULT hr = Swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
    {
        mprinte("Failed to preserve swapchain format during resize!\n");
        return;
    }
    
    ID3D11Texture2D* pBuffer;
    hr = Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**) &pBuffer );
    if (FAILED(hr))
    {
        mprinte("Failed to retrieve the texture for the RenderTarget View!\n");
        return;
    }
    
    hr = Device->CreateRenderTargetView(pBuffer, NULL, RenderTarget);
    if (FAILED(hr))
    {
        mprinte("Failed to create the RenderTarget View!\n");
        return;
    }
    pBuffer->Release();
}

void RendererEntry(frame_params *FrameParams)
{
#if 0
    ID3D11DeviceContext *DeviceContext =
        FrameParams->Resources[Renderer->Device.Index].Device.Context;
    IDXGISwapChain *Swapchain =
        FrameParams->Resources[Renderer->Device.Index].Device.Swapchain;
    ID3D11RenderTargetView *RenderTarget =
        FrameParams->Resources[Renderer->RenderTarget.Index].RenderTarget.Handle;
#else
    
    ID3D11DeviceContext *DeviceContext   = GlobalRenderer->DeviceContext;
    IDXGISwapChain *Swapchain            = GlobalRenderer->Swapchain;
    ID3D11RenderTargetView *RenderTarget = GlobalRenderer->RenderTarget;
    
#endif
    
    DeviceContext->OMSetRenderTargets(1, &RenderTarget, NULL);
    
    // Set the viewport
    D3D11_VIEWPORT Viewport;
    ZeroMemory(&Viewport, sizeof(D3D11_VIEWPORT));
    
    u32 Width, Height;
    PlatformGetClientWindowDimensions(&Width, &Height);
    Viewport.TopLeftX = 0;
    Viewport.TopLeftY = 0;
    Viewport.Width    = Width;
    Viewport.Height   = Height;
    DeviceContext->RSSetViewports(1, &Viewport);
    
    vec4 ClearColor = { 0.0f, 0.2f, 0.4f, 1.0f };
    DeviceContext->ClearRenderTargetView(RenderTarget, ClearColor.data);
    
    for (u32 i = 0; i < FrameParams->RenderCommandsCount; ++i)
    {
        render_command Cmd = FrameParams->RenderCommands[i];
        
        if (Cmd.Type == RenderCmd_Draw)
        {
            asset_id AssetId = Cmd.DrawCmd.Asset;
            
            if (IsValidAsset(FrameParams->Assets, AssetId))
            {
                asset Asset = FrameParams->Assets[AssetId.Index];
                
                if (AssetId.Type == Asset_SimpleModel)
                {
                    resource_id PipelineId = Asset.SimpleModel.Pipeline;
                    resource_id VertexId = Asset.SimpleModel.VertexBuffer;
                    resource_id DiffuseId = Asset.SimpleModel.DiffuseTexture;
                    
                    resource_pipeline Pipeline = FrameParams->Resources[PipelineId.Index].Pipeline;
                    resource_buffer VertexBuffer = FrameParams->Resources[VertexId.Index].Buffer;
                    
                    ID3D11InputLayout  *Layout = Pipeline.Layout;
                    ID3D11VertexShader *VertexShader = Pipeline.VertexShader;
                    ID3D11PixelShader  *PixelShader  = Pipeline.PixelShader;
                    ID3D11Buffer *VBuffer = VertexBuffer.Handle;
                    
                    DeviceContext->IASetInputLayout(Layout);
                    
                    // set the shader objects to be active
                    DeviceContext->VSSetShader(VertexShader, 0, 0);
                    DeviceContext->PSSetShader(PixelShader, 0, 0);
                    
                    // Bind Pixel Shader resources
                    if (IsValidResource(FrameParams->Resources, DiffuseId))
                    {
                        resource_texture DiffuseTexture = FrameParams->Resources[DiffuseId.Index].Texture;
                        
                        DeviceContext->PSSetShaderResources(0, 1, &DiffuseTexture.View);
                    }
                    
                    UINT Stride = Asset.SimpleModel.VertexStride;
                    UINT Offset = Asset.SimpleModel.VertexOffset;
                    DeviceContext->IASetVertexBuffers(0, 1, &VBuffer, &Stride, &Offset);
                    
                    // select which primtive type we are using
                    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    
                    // draw the vertex buffer to the back buffer
                    DeviceContext->Draw(Asset.SimpleModel.VertexCount, 0);
                }
            }
        }
        else if (Cmd.Type == RenderCmd_DrawUi)
        {
            MapleEngineDrawUi(&GlobalRenderer->WindowStack);
        }
        
    }
    
    Swapchain->Present(0, 0);
}
