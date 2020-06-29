
void RendererInit(renderer_t *Renderer, free_allocator *Allocator, resource_registry *Registry,
                  window_t Window, u32 Width, u32 Height, u32 RefreshRate)
{
    renderer_t pRenderer = (renderer_t)FreeListAllocatorAlloc(Allocator, sizeof(renderer));
    
    device_create_info DeviceInfo = {};
    DeviceInfo.Width       = Width;
    DeviceInfo.Height      = Height;
    DeviceInfo.RefreshRate = RefreshRate;
    DeviceInfo.Window      = Window;
    DeviceInfo.SampleCount = 1;
    pRenderer->Device = CreateResource(Registry, Resource_Device, &DeviceInfo);
    
    render_target_create_info RenderTargetInfo = {};
    RenderTargetInfo.Device = pRenderer->Device;
    pRenderer->RenderTarget = CreateResource(Registry, Resource_RenderTarget, &RenderTargetInfo);
    
    *Renderer = pRenderer;
}

void RendererShutdown(renderer_t *Renderer, free_allocator *Allocator)
{
    FreeListAllocatorAllocFree(Allocator, *Renderer);
    *Renderer = NULL;
}

void RendererResize(renderer_t Renderer, resource_registry *Registry)
{
    ID3D11Device *Device = Registry->Resources[Renderer->Device.Index]->Device.Handle;
    ID3D11DeviceContext *DeviceContext = Registry->Resources[Renderer->Device.Index]->Device.Context;
    IDXGISwapChain *Swapchain = Registry->Resources[Renderer->Device.Index]->Device.Swapchain;
    ID3D11RenderTargetView **RenderTarget = &Registry->Resources[Renderer->RenderTarget.Index]->RenderTarget.Handle;
    
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

void RendererEntry(renderer_t Renderer, frame_params *FrameParams)
{
    ID3D11DeviceContext *DeviceContext =
        FrameParams->Resources[Renderer->Device.Index].Device.Context;
    IDXGISwapChain *Swapchain =
        FrameParams->Resources[Renderer->Device.Index].Device.Swapchain;
    ID3D11RenderTargetView *RenderTarget =
        FrameParams->Resources[Renderer->RenderTarget.Index].RenderTarget.Handle;
    
    //ID3D11InputLayout  *Layout       = Registry->Resources[Renderer->SimplePipeline.Index]->Pipeline.Layout;
    //ID3D11VertexShader *VertexShader = Registry->Resources[Renderer->SimplePipeline.Index]->Pipeline.VertexShader;
    //ID3D11PixelShader  *PixelShader  = Registry->Resources[Renderer->SimplePipeline.Index]->Pipeline.PixelShader;
    //ID3D11Buffer *VBuffer = Registry->Resources[Renderer->VertexBuffer.Index]->Buffer.Handle;
    
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
    }
    
    
    // Draw dev gui
    MapleDevGuiNewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    {
        ImGui::ShowDemoWindow();
    }
    ImGui::Render();
    ImDrawData* ImDrawData = ImGui::GetDrawData();
    MapleDevGuiRenderDrawData(ImDrawData);
    ImGui::EndFrame();
    
    
    Swapchain->Present(0, 0);
}
