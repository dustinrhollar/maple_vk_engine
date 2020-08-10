
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
    sd.BufferCount                        = 2;
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
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // allow full-screen switching
    
    D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_1 };
    
    UINT CreationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    // If the project is in a debug build, enable the debug layer.
    CreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    
    HRESULT hr = S_OK;
    D3D_FEATURE_LEVEL FeatureLevel;
    
    hr = D3D11CreateDeviceAndSwapChain(NULL,
                                       D3D_DRIVER_TYPE_HARDWARE,
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
        PlatformFatalError("Failed to create Device and swapchain because of %d!\n", hr);
    }
    
    //~ Create the depth stencil view
    
    D3D11_TEXTURE2D_DESC DepthStencilDesc;
    DepthStencilDesc.Width              = Width;
    DepthStencilDesc.Height             = Height;
    DepthStencilDesc.MipLevels          = 1;
    DepthStencilDesc.ArraySize          = 1;
    DepthStencilDesc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    DepthStencilDesc.SampleDesc.Count   = 1;
    DepthStencilDesc.SampleDesc.Quality = 0;
    DepthStencilDesc.Usage              = D3D11_USAGE_DEFAULT;
    DepthStencilDesc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
    DepthStencilDesc.CPUAccessFlags     = 0; 
    DepthStencilDesc.MiscFlags          = 0;
    
    hr = GlobalRenderer->Device->CreateTexture2D(&DepthStencilDesc, NULL, &GlobalRenderer->DepthStencilBuffer);
    if (FAILED(hr))
    {
        PlatformFatalError("Failed to create depth stencil texture because of %d!\n", hr);
    }
    
    hr = GlobalRenderer->Device->CreateDepthStencilView(GlobalRenderer->DepthStencilBuffer, 
                                                        NULL, 
                                                        &GlobalRenderer->DepthStencilView);
    if (FAILED(hr))
    {
        PlatformFatalError("Failed to create depth stencil view because of %d!\n", hr);
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
        TerrainSettings.Scale           = 0.5f;
        TerrainSettings.Lacunarity      = 1.0f;
        
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
    
    
    //~ Create the rasterizers
    
    D3D11_RASTERIZER_DESC SolidRaster = {};
    SolidRaster.FillMode              = D3D11_FILL_SOLID;
    SolidRaster.CullMode              = D3D11_CULL_BACK;
    SolidRaster.FrontCounterClockwise = TRUE; // NOTE(Dustin): Default is FALSE
    SolidRaster.DepthBias             = 0;
    SolidRaster.DepthBiasClamp        = 0.0f;
    SolidRaster.SlopeScaledDepthBias  = 0.0f;
    SolidRaster.DepthClipEnable       = TRUE;
    SolidRaster.ScissorEnable         = FALSE;
    SolidRaster.MultisampleEnable     = FALSE;
    SolidRaster.AntialiasedLineEnable = FALSE;
    
    hr = GlobalRenderer->Device->CreateRasterizerState(&SolidRaster,
                                                       &GlobalRenderer->SolidRaster);
    
    if (FAILED(hr))
    {
        PlatformFatalError("Failed to create solid rasterizer: %d\n", hr);
    }
    
    D3D11_RASTERIZER_DESC WireframeRaster = {};
    WireframeRaster.FillMode              = D3D11_FILL_WIREFRAME;
    WireframeRaster.CullMode              = D3D11_CULL_BACK;
    WireframeRaster.FrontCounterClockwise = TRUE; // NOTE(Dustin): Default is FALSE
    WireframeRaster.DepthBias             = 0;
    WireframeRaster.DepthBiasClamp        = 0.0f;
    WireframeRaster.SlopeScaledDepthBias  = 0.0f;
    WireframeRaster.DepthClipEnable       = TRUE;
    WireframeRaster.ScissorEnable         = TRUE;
    WireframeRaster.MultisampleEnable     = FALSE;
    WireframeRaster.AntialiasedLineEnable = FALSE;
    
    hr = GlobalRenderer->Device->CreateRasterizerState(&WireframeRaster,
                                                       &GlobalRenderer->WireframeRaster);
    
    if (FAILED(hr))
    {
        PlatformFatalError("Failed to create solid rasterizer: %d\n", hr);
    }
    
}


void RendererShutdown(free_allocator *Allocator)
{
    GlobalRenderer->DepthStencilView->Release();
    GlobalRenderer->DepthStencilBuffer->Release();
    
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
    ID3D11Device *Device                  = GlobalRenderer->Device;
    ID3D11DeviceContext *DeviceContext    = GlobalRenderer->DeviceContext;
    IDXGISwapChain *Swapchain             = GlobalRenderer->Swapchain;
    ID3D11RenderTargetView **RenderTarget = &GlobalRenderer->RenderTarget;
    
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
    ID3D11DeviceContext *DeviceContext   = GlobalRenderer->DeviceContext;
    IDXGISwapChain *Swapchain            = GlobalRenderer->Swapchain;
    ID3D11RenderTargetView *RenderTarget = GlobalRenderer->RenderTarget;
    
    u32 Width, Height;
    PlatformGetClientWindowDimensions(&Width, &Height);
    
    mat4 View  = GetViewMatrix(FrameParams->Camera);
    
    mat4 Projection  = PerspectiveProjection(90.0f, (r32)Width/(r32)Height, 0.1f, 1000.0f);
    //Projection[1][1] *= -1;
    
    mat4 Model = mat4(1.0f); // terrain will generally have a Identity Matrix
    
    mat4 _Mvp = Mul(Projection, Mul(View, Model));
    
    DeviceContext->OMSetRenderTargets(1, &RenderTarget, GlobalRenderer->DepthStencilView);
    
    // Set the viewport
    D3D11_VIEWPORT Viewport;
    ZeroMemory(&Viewport, sizeof(D3D11_VIEWPORT));
    
    Viewport.TopLeftX = 0;
    Viewport.TopLeftY = 0;
    Viewport.Width    = Width;
    Viewport.Height   = Height;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    DeviceContext->RSSetViewports(1, &Viewport);
    
    
    window_rect PlatformRect = PlatformGetClientWindowRect();
    
    D3D11_RECT Scissor = {};
    Scissor.left   = PlatformRect.Left;
    Scissor.right  = PlatformRect.Right;
    Scissor.top    = PlatformRect.Top;
    Scissor.bottom = PlatformRect.Bottom;
    
    DeviceContext->RSSetScissorRects(1, &Scissor);
    
    vec4 ClearColor = { 0.0f, 0.2f, 0.4f, 1.0f };
    DeviceContext->ClearRenderTargetView(RenderTarget, ClearColor.data);
    DeviceContext->ClearDepthStencilView(GlobalRenderer->DepthStencilView, 
                                         D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
    
    DeviceContext->RSSetState(GlobalRenderer->WireframeRaster);
    
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
        else if (Cmd.Type == RenderCmd_DrawTerrain)
        {
            asset_id AssetId = Cmd.DrawCmd.Asset;
            
            if (IsValidAsset(FrameParams->Assets, AssetId))
            {
                asset_terrain Terrain = FrameParams->Assets[AssetId.Index].Terrain;
                
                struct mvp 
                {
                    mat4 Projection;
                    mat4 View;
                    mat4 Model;
                } Mvp;
                
                Mvp.Projection = Projection;
                Mvp.View = View;
                Mvp.Model = Model;
                //Mvp.Model = _Mvp;
                
                resource_id PipelineId = Terrain.Pipeline;
                resource_id VertexId   = Terrain.VertexBuffer;
                resource_id IndexId    = Terrain.IndexBuffer;
                resource_id MvpId      = Terrain.MvpBuffer;
                
                resource_pipeline Pipeline   = FrameParams->Resources[PipelineId.Index].Pipeline;
                resource_buffer VertexBuffer = FrameParams->Resources[VertexId.Index].Buffer;
                resource_buffer IndexBuffer  = FrameParams->Resources[IndexId.Index].Buffer;
                resource_buffer MvpBuffer     = FrameParams->Resources[MvpId.Index].Buffer;
                
                ID3D11InputLayout  *Layout       = Pipeline.Layout;
                ID3D11VertexShader *VertexShader = Pipeline.VertexShader;
                ID3D11PixelShader  *PixelShader  = Pipeline.PixelShader;
                ID3D11Buffer *VBuffer            = VertexBuffer.Handle;
                ID3D11Buffer *IBuffer            = IndexBuffer.Handle;
                ID3D11Buffer *MBuffer            = MvpBuffer.Handle;
                
                DeviceContext->IASetInputLayout(Layout);
                
                // Set the Mvp uniform
                DeviceContext->UpdateSubresource(MBuffer, 0, NULL, &Mvp, 0, 0);
                DeviceContext->VSSetConstantBuffers( 0, 1, &MBuffer);
                
                // set the shader objects to be active
                DeviceContext->VSSetShader(VertexShader, 0, 0);
                DeviceContext->PSSetShader(PixelShader, 0, 0);
                
                UINT Stride = sizeof(terrain_vertex);
                UINT Offset = 0;
                DeviceContext->IASetVertexBuffers(0, 1, &VBuffer, &Stride, &Offset);
                DeviceContext->IASetIndexBuffer(IBuffer, DXGI_FORMAT_R32_UINT, 0);
                
#if 1
                // select which primtive type we are using
                DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                // draw the vertex buffer to the back buffer
                DeviceContext->DrawIndexed(Terrain.IndexCount, 0, 0 );
#else
                // select which primtive type we are using
                DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                // draw the vertex buffer to the back buffer
                DeviceContext->Draw(Terrain.VertexCount, 0);
#endif
            }
        }
        else if (Cmd.Type == RenderCmd_DrawUi)
        {
            MapleEngineDrawUi(&GlobalRenderer->WindowStack);
        }
        
    }
    
    Swapchain->Present(0, 0);
}
