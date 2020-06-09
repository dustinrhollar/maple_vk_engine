
//file_global BufferParameters VertexBuffer;

file_global bool IsGameInit      = false;
file_global bool GameNeedsResize = false;

file_global PerspectiveCamera Camera;

file_internal void RenderAssetMesh(frame_params *FrameParams, mesh *Mesh, mat4 Matrix);
file_internal void RenderAssetNode(frame_params *FrameParams, model_node *Node, mat4 Matrix);
file_internal void RenderAllAssets(frame_params *FrameParams);
file_internal void ProcessKeyboardInput(void *instance, KeyPressEvent event);

void GameStageInit(frame_params* FrameParams)
{
    //~ Camera
    vec3 pos = {0.0f, 0.0f, -2.0f};
    InitPerspectiveCamera(&Camera, pos);
    
    //~ Asset Loading
    
#if 1
    //jstring ExampleGltfModel = InitJString("data/glTF/Fox/glTF/fox.gltf");
    jstring ExampleGltfModel = InitJString("data/glTF/Lantern/Lantern.gltf");
    
    masset::ConvertGlTF(ExampleGltfModel);
    ExampleGltfModel.Clear();
#endif
    
#if 1
    //jstring ExampleModel = InitJString("data/models/models/fox.model");
    jstring ExampleModel = InitJString("data/models/models/Lantern.model");
    
    model_create_info *ModelCreateInfo = talloc<model_create_info>(1);
    ModelCreateInfo->Filename          = ExampleModel;
    ModelCreateInfo->FrameParams       = FrameParams;
    masset::Load(Asset_Model, ModelCreateInfo);
    
    ExampleModel.Clear();
#endif
    
    //~ Subscribe to necessary events
    event::Subscribe<KeyPressEvent>(&ProcessKeyboardInput, nullptr);
}

void GameStageEntry(frame_params* FrameParams)
{
    // Prep the frame for rendering
    u32 Width, Height;
    PlatformGetClientWindowDimensions(&Width, &Height);
    VkExtent2D Extent = vk::GetSwapChainExtent();
    
    mat4 Projection = PerspectiveProjection(90.0f, (r32)Width/(r32)Height, 0.1f, 1000.0f);
    Projection[1][1] *= -1;
    
    gpu_begin_frame_info *BeginFrame = talloc<gpu_begin_frame_info>(1);
    BeginFrame->Color    = {0.67f, 0.85f, 0.90f, 1.0f};
    BeginFrame->HasDepth = true;
    BeginFrame->Depth    = 1.0f;
    BeginFrame->Stencil  = 0;
    BeginFrame->GlobalShaderData.Projection = Projection;
    BeginFrame->GlobalShaderData.View       = GetViewMatrix(&Camera);
    AddGpuCommand(FrameParams, { GpuCmd_BeginFrame, BeginFrame });
    
    render_set_scissor_info *ScissorInfo = talloc<render_set_scissor_info>(1);
    ScissorInfo->Extent  = Extent;
    ScissorInfo->XOffset = 0;
    ScissorInfo->YOffset = 0;
    AddRenderCommand(FrameParams, { RenderCmd_SetScissor, ScissorInfo });
    
    render_set_viewport_info *ViewportInfo = talloc<render_set_viewport_info>(1);
    ViewportInfo->Width  = static_cast<r32>(Extent.width);
    ViewportInfo->Height = static_cast<r32>(Extent.height);
    ViewportInfo->X      = 0.0f;
    ViewportInfo->Y      = 0.0f;
    AddRenderCommand(FrameParams, { RenderCmd_SetViewport, ViewportInfo });
    
    RenderAllAssets(FrameParams);
}

void GameStageShutdown(frame_params* FrameParams)
{
}

file_internal void RenderAssetMesh(frame_params *FrameParams, mesh *Mesh, mat4 Matrix)
{
    for (u32 Idx = 0; Idx < Mesh->PrimitivesCount; ++Idx)
    {
        primitive *Primitive = Mesh->Primitives[Idx];
        
        resource_id_t *VBuffers = talloc<resource_id_t>(1);
        u64 *VOffsets = talloc<u64>(1);
        
        VBuffers[0] = Primitive->VertexBuffer;
        VOffsets[0] = 0;
        
        render_draw_command *DrawCommand = talloc<render_draw_command>(1);
        DrawCommand->VertexBuffers      = VBuffers;
        DrawCommand->VertexBuffersCount = 1;
        DrawCommand->Offsets            = VOffsets;
        DrawCommand->IsIndexed          = Primitive->IsIndexed;
        DrawCommand->IndexBuffer        = Primitive->IndexBuffer;
        DrawCommand->Count              = (Primitive->IsIndexed) ? Primitive->IndexCount : Primitive->VertexCount;
        DrawCommand->Material           = Primitive->Material;
        
        object_shader_data ObjectData = {};
        ObjectData.Model = Matrix;
        DrawCommand->ObjectShaderData   = ObjectData;
        
        AddRenderCommand(FrameParams, { RenderCmd_Draw, DrawCommand });
    }
}

file_internal void RenderAssetNode(frame_params *FrameParams, model_node *Node, mat4 Matrix)
{
    mat4 NodeMatrix = mat4(1.0f);
    
    mat4 TranslationMatrix = mat4(1.0f);
    mat4 ScaleMatrix       = mat4(1.0f);
    mat4 RotationMatrix    = mat4(1.0f);
    
    {
        TranslationMatrix = Translate(Node->Translation);
    }
    
    {
        ScaleMatrix = Scale(Node->Scale.x, Node->Scale.y, Node->Scale.z);
    }
    
    {
        vec3 axis = Node->Rotation.xyz;
        float theta = Node->Rotation.w;
        
        Quaternion rotation = MakeQuaternion(axis.x,axis.y,axis.z,theta);
        RotationMatrix = GetQuaternionRotationMatrix(rotation);
    }
    
    // Multiplication order = T * R * S
    NodeMatrix = Mul(NodeMatrix, ScaleMatrix);
    NodeMatrix = Mul(NodeMatrix, RotationMatrix);
    NodeMatrix = Mul(NodeMatrix, TranslationMatrix);
    
    mat4 ModelMatrix = Mul(Matrix, NodeMatrix);
    
    if (Node->Mesh)
    {
        RenderAssetMesh(FrameParams, Node->Mesh, ModelMatrix);
    }
    
    for (int i = 0; i < Node->ChildrenCount; ++i)
    {
        RenderAssetNode(FrameParams, Node->Children[i], ModelMatrix);
    }
}

file_internal void RenderAllAssets(frame_params *FrameParams)
{
    for (u32 AssetIdx = 0; AssetIdx < FrameParams->ModelAssetsCount; ++AssetIdx)
    {
        asset Asset = FrameParams->ModelAssets[AssetIdx];
        
        for (u32 DisjointNode = 0; DisjointNode < Asset.Model.RootModelNodesCount; ++DisjointNode)
        {
            model_node *RootNode = Asset.Model.RootModelNodes[DisjointNode];
            RenderAssetNode(FrameParams, RootNode, mat4(1.0f));
        }
    }
}

file_internal void ProcessKeyboardInput(void *instance, KeyPressEvent event)
{
    EventKey ki = event.Key;
    
    // HACK(Dustin): hardcoded time-step. need a better solution
    r32 time = 0.016667;
    
    r32 delta_x = 0.0f;
    r32 delta_y = 0.0f;
    
    if (ki == KEY_Up)
    {
        delta_y -= time;
    }
    
    if (ki == KEY_Down)
    {
        delta_y += time;
    }
    
    if (ki == KEY_Left)
    {
        delta_x -= time;
    }
    
    if (ki == KEY_Right)
    {
        delta_x += time;
    }
    
    if (delta_x != 0.0f || delta_y != 0.0f)
    {
        r32 xoffset = (delta_x - Camera.MouseXPos);
        r32 yoffset = (delta_y - Camera.MouseYPos);
        
        xoffset *= Camera.MouseSensitivity;
        yoffset *= Camera.MouseSensitivity;
        
        vec2 mouse_rotation = {xoffset, yoffset};
        
        RotateCameraAboutX(&Camera, mouse_rotation.y);
        RotateCameraAboutY(&Camera, -mouse_rotation.x);
    }
    
    r32 velocity = Camera.MovementSpeed * time;
    if (ki == KEY_w)
    {
        Camera.Position += Camera.Front * velocity;
    }
    
    if (ki == KEY_s)
    {
        Camera.Position -= Camera.Front * velocity;
    }
    
    if (ki == KEY_a)
    {
        Camera.Position -= Camera.Right * velocity;
    }
    
    if (ki == KEY_d)
    {
        Camera.Position += Camera.Right * velocity;
    }
    
    UpdatePerspectiveCameraVectors(&Camera);
}

// TODO(Dustin): Move this to the engine layer. This should be Engine related UI
file_internal void CreateVulkanResizableState()
{
#if 0
    // Create ImGui State
    {
        //~ ImGui
        
        // NOTE(Dustin): Is QueueFamily supposed to be the graphics or present queue?
        ImGui_ImplVulkan_InitInfo impl_vk_info = {};
        impl_vk_info.Instance       = vk::GlobalVulkanState.Instance;
        impl_vk_info.PhysicalDevice = vk::GlobalVulkanState.PhysicalDevice;
        impl_vk_info.Device         = vk::GlobalVulkanState.Device;
        impl_vk_info.QueueFamily    = vk::GlobalVulkanState.GraphicsQueue.FamilyIndex;
        impl_vk_info.Queue          = vk::GlobalVulkanState.GraphicsQueue.Handle;
        impl_vk_info.DescriptorPool = DescriptorPool;
        impl_vk_info.MinImageCount  = vk::GetSwapChainImageCount();
        impl_vk_info.ImageCount     = vk::GetSwapChainImageCount();
        impl_vk_info.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;
        
        ImGui_ImplVulkan_Init(&impl_vk_info, RenderPass);
        
        VkCommandBuffer command_buffer = vk::BeginSingleTimeCommands(CommandPool);
        {
            ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
        }
        vk::EndSingleTimeCommands(command_buffer, CommandPool);
    }
    
    //~ Drawing the UI Window
    masset::Render(ModelAsset);
    
    // Render ImGui Window
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    
    ImGui::Render();
    
    // Render draw data into a single time command buffer -
    // not the best idea
    // NOTE(Dustin): DONT DO THAT, but for now I am testing this...
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
    
    ImGui::EndFrame();
    
    //~ Shutting down the UI layer
    ImGui_ImplVulkan_Shutdown();
    
    //~ Game Resize
    event::Subscribe<ResizeEvent>(&GameResize, nullptr);
#endif
}
