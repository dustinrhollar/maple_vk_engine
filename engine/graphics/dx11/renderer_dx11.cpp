
struct renderer
{
    resource_id Device;
    resource_id RenderTarget;
    
    // Temporary Code
    resource_id VertexBuffer;
    resource_id SimplePipeline;
};

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
    
    
    pipeline_layout_create_info LayoutInfo[2] = {
        {"POSITION", 0, PipelineFormat_R32G32B32_FLOAT,    0, 0,  true, 0},
        {"COLOR",    0, PipelineFormat_R32G32B32A32_FLOAT, 0, 12, true, 0},
    };
    
    
    file_t VertFile = PlatformLoadFile("data/shaders/simple_vert.cso");
    file_t FragFile = PlatformLoadFile("data/shaders/simple_frag.cso");
    
    pipeline_create_info PipelineInfo = {};
    PipelineInfo.Device              = pRenderer->Device;
    PipelineInfo.VertexData          = GetFileBuffer(VertFile);
    PipelineInfo.PixelData           = GetFileBuffer(FragFile);
    PipelineInfo.VertexDataSize      = PlatformGetFileSize(VertFile);
    PipelineInfo.PixelDataSize       = PlatformGetFileSize(FragFile);
    PipelineInfo.PipelineLayout      = LayoutInfo;
    PipelineInfo.PipelineLayoutCount = 2;
    pRenderer->SimplePipeline = CreateResource(Registry, Resource_Pipeline, &PipelineInfo);
    
    simple_vertex Vertices[] =
    {
        { {0.0f, 0.5f, 0.0f    } , { 1.0f, 0.0f, 0.0f, 1.0f } },
        { {0.45f, -0.5, 0.0f   } , { 0.0f, 1.0f, 0.0f, 1.0f } },
        { {-0.45f, -0.5f, 0.0f } , { 0.0f, 0.0f, 1.0f, 1.0f } }
    };
    
    buffer_create_info BufferInfo = {};
    BufferInfo.Device              = pRenderer->Device;
    BufferInfo.Size                = sizeof(simple_vertex) * 3;
    BufferInfo.Usage               = BufferUsage_Default;
    BufferInfo.CpuAccessFlags      = BufferCpuAccess_None;
    BufferInfo.BindFlags           = BufferBind_VertexBuffer;
    BufferInfo.MiscFlags           = BufferMisc_None;
    BufferInfo.StructureByteStride = 0;
    BufferInfo.Data                = Vertices;
    BufferInfo.SysMemPitch         = 0;
    BufferInfo.SysMemSlicePitch    = 0;
    pRenderer->VertexBuffer = CreateResource(Registry, Resource_Buffer, &BufferInfo);
    
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

void RendererEntry(renderer_t Renderer, resource_registry *Registry)
{
    ID3D11DeviceContext *DeviceContext = Registry->Resources[Renderer->Device.Index]->Device.Context;
    IDXGISwapChain *Swapchain = Registry->Resources[Renderer->Device.Index]->Device.Swapchain;
    ID3D11RenderTargetView *RenderTarget = Registry->Resources[Renderer->RenderTarget.Index]->RenderTarget.Handle;
    ID3D11InputLayout  *Layout       = Registry->Resources[Renderer->SimplePipeline.Index]->Pipeline.Layout;
    ID3D11VertexShader *VertexShader = Registry->Resources[Renderer->SimplePipeline.Index]->Pipeline.VertexShader;
    ID3D11PixelShader  *PixelShader  = Registry->Resources[Renderer->SimplePipeline.Index]->Pipeline.PixelShader;
    ID3D11Buffer *VBuffer = Registry->Resources[Renderer->VertexBuffer.Index]->Buffer.Handle;
    
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
    
    DeviceContext->IASetInputLayout(Layout);
    
    // set the shader objects to be active
    DeviceContext->VSSetShader(VertexShader, 0, 0);
    DeviceContext->PSSetShader(PixelShader, 0, 0);
    
    UINT Stride = sizeof(simple_vertex);
    UINT Offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &VBuffer, &Stride, &Offset);
    
    // select which primtive type we are using
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // draw the vertex buffer to the back buffer
    DeviceContext->Draw(3, 0);
    
    Swapchain->Present(0, 0);
}
