
// TODO(Dustin): Go to resources
file_global resource_id_t GlobalDescriptorLayout;       // per frame descriptor
file_global resource_id_t GlobalObjectDescriptorLayout; // per object descriptor
file_global resource_id_t GlobalDescriptorSet;
file_global resource_id_t GlobalObjectDescriptorSet;
file_global resource_id_t GlobalPipeline;
file_global resource_id_t GlobalVPBuffer;
file_global resource_id_t GlobalModelBuffer;

//file_global BufferParameters VertexBuffer;

file_global bool IsGameInit      = false;
file_global bool GameNeedsResize = false;

file_global jstring VERT_SHADER;
file_global jstring FRAG_SHADER;

file_global PerspectiveCamera Camera;

file_global asset_id_t ModelAsset;

file_internal void RenderAssetMesh(frame_params *FrameParams, mesh *Mesh, mat4 Matrix,
                                   dyn_uniform_template *PerObjectTemplate);
file_internal void RenderAssetNode(frame_params *FrameParams, model_node *Node, mat4 Matrix,
                                   dyn_uniform_template *PerObjectTemplate);
file_internal void RenderAllAssets(frame_params *FrameParams, dyn_uniform_template *PerObjectTemplate);
file_internal void ProcessKeyboardInput(void *instance, KeyPressEvent event);

void GameStageInit(frame_params* FrameParams)
{
    VERT_SHADER = InitJString("data/shaders/shader.vert.spv");
    FRAG_SHADER = InitJString("data/shaders/shader.frag.spv");
    
    //~ Camera
    vec3 pos = {0.0f, 0.0f, -2.0f};
    InitPerspectiveCamera(&Camera, pos);
    
    //~ Mvp Uniforms
    buffer_create_info MvpBufferCreateInfo = {};
    MvpBufferCreateInfo.BufferSize         = sizeof(mat4) * 2;
    MvpBufferCreateInfo.PersistentlyMapped = true;
    MvpBufferCreateInfo.Usage              = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    MvpBufferCreateInfo.MemoryUsage        = VMA_MEMORY_USAGE_CPU_TO_GPU;
    MvpBufferCreateInfo.MemoryFlags        = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    GlobalVPBuffer = mresource::Load(FrameParams, Resource_UniformBuffer, &MvpBufferCreateInfo);
    
    dynamic_buffer_create_info ModelBufferCreateInfo = {};
    ModelBufferCreateInfo.ElementCount       = 50;
    ModelBufferCreateInfo.ElementStride      = sizeof(mat4);
    ModelBufferCreateInfo.PersistentlyMapped = true;
    ModelBufferCreateInfo.Usage              = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    ModelBufferCreateInfo.MemoryUsage        = VMA_MEMORY_USAGE_CPU_TO_GPU;
    ModelBufferCreateInfo.MemoryFlags        = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    GlobalModelBuffer = mresource::Load(FrameParams, Resource_DynamicUniformBuffer, &ModelBufferCreateInfo);
    
    //~ Descriptor Layouts
    VkDescriptorSetLayoutBinding Bindings[1]= {};
    Bindings[0].binding            = 0;
    Bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    Bindings[0].descriptorCount    = 1;
    Bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    Bindings[0].pImmutableSamplers = nullptr; // Optional
    
    descriptor_layout_create_info DescriptorLayoutCreateInfo = {};
    DescriptorLayoutCreateInfo.BindingsCount = 1;
    DescriptorLayoutCreateInfo.Bindings      = Bindings;
    
    GlobalDescriptorLayout = mresource::Load(FrameParams, Resource_DescriptorSetLayout,
                                             &DescriptorLayoutCreateInfo);
    
    Bindings[0].binding            = 0;
    Bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    Bindings[0].descriptorCount    = 1;
    Bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    Bindings[0].pImmutableSamplers = nullptr; // Optional
    
    DescriptorLayoutCreateInfo.BindingsCount = 1;
    DescriptorLayoutCreateInfo.Bindings      = Bindings;
    
    GlobalObjectDescriptorLayout = mresource::Load(FrameParams, Resource_DescriptorSetLayout,
                                                   &DescriptorLayoutCreateInfo);
    
    //~ Descriptors
    // per frame descriptor
    descriptor_create_info DescriptorCreateInfo = {};
    DescriptorCreateInfo.DescriptorLayouts = &GlobalDescriptorLayout;
    DescriptorCreateInfo.SetCount          = 1;
    
    GlobalDescriptorSet = mresource::Load(FrameParams, Resource_DescriptorSet,
                                          &DescriptorCreateInfo);
    
    // per object descriptor
    DescriptorCreateInfo.DescriptorLayouts = &GlobalObjectDescriptorLayout;
    DescriptorCreateInfo.SetCount          = 1;
    
    GlobalObjectDescriptorSet = mresource::Load(FrameParams, Resource_DescriptorSet,
                                                &DescriptorCreateInfo);
    
    // Bind the uniform buffers to the descriptors
    descriptor_write_info WriteInfos[2] = {
        { GlobalVPBuffer,    GlobalDescriptorSet,       0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER         },
        { GlobalModelBuffer, GlobalObjectDescriptorSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC }
    };
    
    descriptor_update_write_info DescriptorUpdateInfos = {};
    DescriptorUpdateInfos.WriteInfos      = WriteInfos;
    DescriptorUpdateInfos.WriteInfosCount = 2;
    
    mresource::Load(FrameParams, Resource_DescriptorSetWriteUpdate, &DescriptorUpdateInfos);
    
    //~ Pipeline
    shader_file_create_info Shaders[] = {
        { VERT_SHADER, VK_SHADER_STAGE_VERTEX_BIT   },
        { FRAG_SHADER, VK_SHADER_STAGE_FRAGMENT_BIT }
    };
    
    VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();
    JTuple<VkVertexInputAttributeDescription*, int> attribs = Vertex::GetAttributeDescriptions();
    
    VkExtent2D swapchain_extent = vk::GetSwapChainExtent();
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = (r32) swapchain_extent.width;
    viewport.height = (r32) swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent;
    
    resource_id_t Layouts[] = {
        GlobalDescriptorLayout,
        GlobalObjectDescriptorLayout,
    };
    
    pipeline_create_info PipelineCreateInfo = {};
    PipelineCreateInfo.Shaders      = Shaders;
    PipelineCreateInfo.ShadersCount = 2;
    PipelineCreateInfo.VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    PipelineCreateInfo.VertexInputInfo.vertexBindingDescriptionCount   = 1;
    PipelineCreateInfo.VertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    PipelineCreateInfo.VertexInputInfo.vertexAttributeDescriptionCount = attribs.Second();
    PipelineCreateInfo.VertexInputInfo.pVertexAttributeDescriptions    = attribs.First();
    PipelineCreateInfo.Viewport               = &viewport;
    PipelineCreateInfo.Scissor                = &scissor;
    PipelineCreateInfo.ScissorCount           = 1;
    PipelineCreateInfo.ViewportCount          = 1;
    PipelineCreateInfo.PolygonMode            = VK_POLYGON_MODE_FILL;
    PipelineCreateInfo.FrontFace              = VK_FRONT_FACE_CLOCKWISE;
    PipelineCreateInfo.HasMultisampling       = false;
    PipelineCreateInfo.MuliSampleSamples      = VK_SAMPLE_COUNT_1_BIT; // optional
    PipelineCreateInfo.HasDepthStencil        = true;
    PipelineCreateInfo.DescriptorLayoutIds    = Layouts;
    PipelineCreateInfo.DescriptorLayoutsCount = 2;
    PipelineCreateInfo.PushConstants          = nullptr;
    PipelineCreateInfo.PushConstantsCount     = 0;
    PipelineCreateInfo.RenderPass             = GetPrimaryRenderPass();
    
    GlobalPipeline = mresource::Load(FrameParams, Resource_Pipeline, &PipelineCreateInfo);
    
    //~ Asset Loading
    jstring ExampleModel = InitJString("data/models/Fox/glTF/fox.gltf");
    
    model_create_info *ModelCreateInfo = talloc<model_create_info>(1);
    ModelCreateInfo->Filename    = ExampleModel;
    ModelCreateInfo->FrameParams = FrameParams;
    ModelAsset = masset::Load(Asset_Model, ModelCreateInfo);
    
    masset::ConvertGlTF(ExampleModel);
    
    ExampleModel.Clear();
    
    //~ Subscribe to necessary events
    event::Subscribe<KeyPressEvent>(&ProcessKeyboardInput, nullptr);
}

void GameStageEntry(frame_params* FrameParams)
{
    VkExtent2D Extent = vk::GetSwapChainExtent();
    
    gpu_begin_frame_info *BeginFrame = talloc<gpu_begin_frame_info>(1);
    BeginFrame->Color    = {0.67f, 0.85f, 0.90f, 1.0f};
    BeginFrame->HasDepth = true;
    BeginFrame->Depth    = 1.0f;
    BeginFrame->Stencil  = 0;
    AddGpuCommand(FrameParams, { GpuCmd_BeginFrame, BeginFrame });
    
    // Copy Per-Frame Descriptors into Uniform memory
    {
        u32 Width, Height;
        PlatformGetClientWindowDimensions(&Width, &Height);
        
        mat4 View = GetViewMatrix(&Camera);
        mat4 Proj = PerspectiveProjection(90.0f, (r32)Width/(r32)Height, 0.1f, 1000.0f);
        
        struct vp {
            mat4 View;
            mat4 Proj;
        } *ViewProj = talloc<vp>(1);
        
        ViewProj->View = View;
        ViewProj->Proj = Proj;
        ViewProj->Proj[1][1] *= -1;
        
        gpu_update_buffer_info *BufferInfo = talloc<gpu_update_buffer_info>(1);
        BufferInfo->Uniform        = GlobalVPBuffer;
        BufferInfo->Data           = ViewProj;
        BufferInfo->DataSize       = sizeof(vp);
        BufferInfo->BufferOffset   = 0;
        
        AddGpuCommand(FrameParams, { GpuCmd_UpdateBuffer, BufferInfo });
    }
    
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
    
    // NOTE(Dustin): ok....so there is only one pipeline so go ahead and bind it...
    render_bind_pipeline_info *PipelineBindInfo = talloc<render_bind_pipeline_info>(1);
    PipelineBindInfo->PipelineId = GlobalPipeline;
    AddRenderCommand(FrameParams, { RenderCmd_BindPipeline, PipelineBindInfo });
    
    // Bind Global Descriptor
    render_bind_descriptor_set *BindDescriptor = talloc<render_bind_descriptor_set>(1);
    BindDescriptor->PipelineId          = GlobalPipeline;
    BindDescriptor->DescriptorId        = GlobalDescriptorSet;
    BindDescriptor->FirstSet            = 0;
    BindDescriptor->DynamicOffsets      = nullptr;
    BindDescriptor->DynamicOffsetsCount = 0;
    AddRenderCommand(FrameParams, { RenderCmd_BindDescriptorSet, BindDescriptor });
    
    // TODO(Dustin): Parse assets*
    dyn_uniform_template PerObjectTemplate = mresource::GetDynamicUniformTemplate(GlobalObjectDescriptorSet);
    
    RenderAllAssets(FrameParams, &PerObjectTemplate);
}

void GameStageShutdown(frame_params* FrameParams)
{
    VERT_SHADER.Clear();
    FRAG_SHADER.Clear();
}

file_internal void RenderAssetMesh(frame_params *FrameParams, mesh *Mesh, mat4 Matrix,
                                   dyn_uniform_template *PerObjectTemplate)
{
    // Update Model Info and bind the object descriptor
    i64 ObjOffset = mresource::DynUniformGetNextOffset(PerObjectTemplate);
    
    mat4 *tMat = talloc<mat4>(1);
    *tMat = Matrix;
    
    gpu_update_buffer_info *BufferInfo = talloc<gpu_update_buffer_info>(1);
    BufferInfo->Uniform        = GlobalModelBuffer;
    BufferInfo->Data           = tMat;
    BufferInfo->DataSize       = sizeof(mat4);
    BufferInfo->BufferOffset   = ObjOffset;
    AddGpuCommand(FrameParams, { GpuCmd_UpdateBuffer, BufferInfo });
    
    u32 *Offsets = talloc<u32>(1);
    Offsets[0] = ObjOffset;
    
    render_bind_descriptor_set *BindDescriptor = talloc<render_bind_descriptor_set>(1);
    BindDescriptor->PipelineId          = GlobalPipeline;
    BindDescriptor->DescriptorId        = GlobalObjectDescriptorSet;
    BindDescriptor->FirstSet            = 1;
    BindDescriptor->DynamicOffsets      = Offsets;
    BindDescriptor->DynamicOffsetsCount = 1;
    AddRenderCommand(FrameParams, { RenderCmd_BindDescriptorSet, BindDescriptor });
    
    for (int i = 0; i < Mesh->PrimitivesCount; ++i)
    {
        primitive Primitive = Mesh->Primitives[i];
        
        resource_id_t *VBuffers = talloc<resource_id_t>(1);
        u64*VOffsets = talloc<u64>(1);
        
        VBuffers[0] = Primitive.VertexBuffer;
        VOffsets[0] = 0;
        
        render_draw_command *DrawCommand = talloc<render_draw_command>(1);
        DrawCommand->VertexBuffers      = VBuffers;
        DrawCommand->VertexBuffersCount = 1;
        DrawCommand->Offsets            = VOffsets;
        DrawCommand->IsIndexed          = Primitive.IsIndexed;
        DrawCommand->IndexBuffer        = Primitive.IndexBuffer;
        DrawCommand->Count              = (Primitive.IsIndexed) ? Primitive.IndexCount : Primitive.VertexCount;
        AddRenderCommand(FrameParams, { RenderCmd_Draw, DrawCommand });
    }
}

file_internal void RenderAssetNode(frame_params *FrameParams, model_node *Node, mat4 Matrix,
                                   dyn_uniform_template *PerObjectTemplate)
{
    mat4 NodeMatrix = mat4(1.0f);
    
    if (Node->HasMatrix)
    {
        NodeMatrix = Node->Matrix;
    }
    else
    {
        mat4 TranslationMatrix = mat4(1.0f);
        mat4 ScaleMatrix       = mat4(1.0f);
        mat4 RotationMatrix    = mat4(1.0f);
        
        if (Node->HasTranslation)
        {
            TranslationMatrix = Translate(Node->Translation);
        }
        
        if (Node->HasScale)
        {
            ScaleMatrix = Scale(Node->Scale.x, Node->Scale.y, Node->Scale.z);
        }
        
        if (Node->HasRotation)
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
    }
    
    mat4 ModelMatrix = Mul(Matrix, NodeMatrix);
    
    if (Node->Mesh)
    {
        RenderAssetMesh(FrameParams, Node->Mesh, ModelMatrix, PerObjectTemplate);
    }
    
    for (int i = 0; i < Node->ChildrenCount; ++i)
    {
        RenderAssetNode(FrameParams, Node->Children + i, ModelMatrix, PerObjectTemplate);
    }
}

file_internal void RenderAllAssets(frame_params *FrameParams, dyn_uniform_template *PerObjectTemplate)
{
    for (u32 AssetIdx = 0; AssetIdx < FrameParams->AssetsCount; ++AssetIdx)
    {
        asset Asset = FrameParams->Assets[AssetIdx];
        
        if (Asset.Type == Asset_Model)
        {
            for (u32 DisjointNode = 0; DisjointNode < Asset.Model.Model.NodesCount; ++DisjointNode)
            {
                model_node *RootNode = Asset.Model.Model.Nodes + DisjointNode;
                RenderAssetNode(FrameParams, RootNode, mat4(1.0f), PerObjectTemplate);
            }
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
