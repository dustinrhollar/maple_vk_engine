
// TODO(Dustin): Go to resources

// TODO(Dustin): Go to resources

// TODO(Dustin): Go to resources
file_global resource_id_t GlobalDescriptorLayout;
file_global resource_id_t GlobalDescriptorSet;
file_global resource_id_t GlobalPipeline;
//file_global uniform               *MvpUniforms;  // one per swapchain image

//file_global BufferParameters VertexBuffer;

file_global bool IsGameInit      = false;
file_global bool GameNeedsResize = false;

file_global jstring EXE_PATH;
file_global jstring SHADER_PATH;

file_global jstring VERT_SHADER;
file_global jstring FRAG_SHADER;

file_global PerspectiveCamera Camera;

file_global asset_id_t ModelAsset;

file_internal void RecordCommandBuffer(u32 idx);
file_internal void CreateVulkanResizableState();

void GameStageInit(frame_params FrameParams)
{
    // TODO(Dustin): Currently, a user will have know the swapchain image
    // count when allocating the descriptors...I don't like this. I'd rather
    // the descriptor count be hidden to make resize easier to manager. Let
    // a user create a single descriptor, and the Resource Manager create that
    // descriptor/uniform for each swapchain image.
    
    u32 SwapchainImageCount = vk::GetSwapChainImageCount();
    
    //~ Camera
    vec3 pos = {0.0f, 0.0f, -2.0f};
    InitPerspectiveCamera(&Camera, pos);
    
    //~ Mvp Uniforms
    buffer_create_info MvpBufferCreateInfo = {};
    MvpBufferCreateInfo.SizePerBuffer      = sizeof(mat4) * 3;
    MvpBufferCreateInfo.BufferCount        = SwapchainImageCount;
    MvpBufferCreateInfo.PersistentlyMapped = true;
    MvpBufferCreateInfo.Usage              = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    MvpBufferCreateInfo.Properties         = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
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
    
    GlobalDescriptorLayout = mresource::Load(Resource_DescriptorSetLayout,
                                             &DescriptorLayoutCreateInfo);
    
    //~ Descriptors
    resource_id_t *Layouts = talloc<resource_id_t>(SwapchainImageCount);
    for (u32 Layout = 0; Layout < SwapchainImageCount; ++Layout)
        Layouts[Layout] = GlobalDescriptorLayout;
    
    descriptor_create_info DescriptorCreateInfo = {};
    DescriptorCreateInfo.DescriptorLayouts = Layouts;
    DescriptorCreateInfo.SetCount          = SwapchainImageCount;
    
    GlobalDescriptorSet = mresource::Load(Resource_DescriptorSet,
                                          &DescriptorCreateInfo);
    
    //~ Pipeline
    VERT_SHADER = InitJString("data/shaders/shader.vert.spv");
    FRAG_SHADER = InitJString("data/shaders/shader.frag.spv");
    
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
    viewport.width  = (float) swapchain_extent.width;
    viewport.height = (float) swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent;
    
    // TODO(Dustin): Need to get the Descriptor layouts and Render Pass
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
    //PipelineCreateInfo.MuliSampleSamples; // optional
    PipelineCreateInfo.HasDepthStencil        = true;
    PipelineCreateInfo.DescriptorLayoutIds    = Layouts;
    PipelineCreateInfo.DescriptorLayoutsCount = 1;
    PipelineCreateInfo.PushConstants          = nullptr;
    PipelineCreateInfo.PushConstantsCount     = 0;
    //PipelineCreateInfo.RenderPass             = ;
    
    GlobalPipeline = mresource::Load(Resource_Pipeline, &PipelineCreateInfo);
    
    // Build any render commands
    
}

void GameStageEntry(frame_params FrameParams)
{
}

void GameStageShutdown(frame_params FrameParams)
{
}

file_internal void CreateVulkanResizableState()
{
#if 0
    u32 swapchain_image_count       = vk::GetSwapChainImageCount();
    VkFormat swapchain_image_format = vk::GetSwapChainImageFormat();
    VkExtent2D extent               = vk::GetSwapChainExtent();
    
    { // Create Global Descriptors
        for (u32 i = 0; i < swapchain_image_count; ++i)
        {
            VkWriteDescriptorSet descriptorWrites[1] = {};
            
            // Set 3: Material Information
            VkDescriptorBufferInfo uniform_info = {};
            uniform_info.buffer                 = MvpUniforms[i].Buffer.Handle;
            uniform_info.offset                 = 0;
            uniform_info.range                  = VK_WHOLE_SIZE;
            
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet           = GlobalDescriptorSets[i];
            descriptorWrites[0].dstBinding       = 0;
            descriptorWrites[0].dstArrayElement  = 0;
            descriptorWrites[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount  = 1;
            descriptorWrites[0].pBufferInfo      = &uniform_info;
            descriptorWrites[0].pImageInfo       = nullptr;
            descriptorWrites[0].pTexelBufferView = nullptr;
            
            vk::UpdateDescriptorSets(descriptorWrites, 1);
        }
    }
    
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
#endif
}

// TODO(Dustin): Move to Resources...

void GameResize(void *instance, ResizeEvent event)
{
#if 0
    if (!IsGameInit) return;
    
    ImGui_ImplVulkan_Shutdown();
    
    vk::DestroyDescriptorPool(DescriptorPool);
    
    vk::DestroyCommandBuffers(CommandPool, CommandBuffersCount,
                              CommandBuffers);
    pfree(CommandBuffers);
    
    // Destroy framebuffer
    for (u32 i = 0; i < FramebufferCount; ++i)
    {
        vk::DestroyFramebuffer(Framebuffer[i]);
    }
    pfree(Framebuffer);
    
    vk::DestroyRenderPass(RenderPass);
    
    CreateVulkanResizableState();
#endif
}


void FlagGameResize()
{
    GameNeedsResize = true;
}


void GameUpdateAndRender(FrameInput input)
{
    mm::ResetTransientMemory();
    
#if 0
    u32 image_index;
    
    VkResult khr_result = vk::BeginFrame(image_index);
    if (khr_result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        u32 width, height;
        PlatformGetClientWindowDimensions(&width, &height);
        
        WindowResizeEvent event;
        event.Width  = width;
        event.Height = height;
        
        event::Dispatch<WindowResizeEvent>(event);
        return;
    }
    else if (khr_result != VK_SUCCESS &&
             khr_result != VK_SUBOPTIMAL_KHR)
    {
        mprinte("Failed to acquire swap chain image!");
    }
    
    {// Update uniform data
        uniform *FrameUniform = &MvpUniforms[image_index];
        void *uniform_ptr = (FrameUniform->AllocInfo.pMappedData);
        
        u32 width, height;
        PlatformGetClientWindowDimensions(&width, &height);
        mat4 proj  = PerspectiveProjection(90.0f, (float)width/(float)height, 0.1f, 4000.0f);
        mat4 view  = GetViewMatrix(&Camera);
        mat4 model = mat4(1.0f);
        
        struct MVP {
            alignas(16) mat4 View;
            alignas(16) mat4 Proj;
            alignas(16) mat4 Model;
        } mvp;
        
        mvp.View = view;
        mvp.Proj = proj;
        mvp.Proj[1][1] *= -1;
        mvp.Model = model;
        
        memcpy(uniform_ptr, &mvp, sizeof(MVP));
    }
    
    // Command buffer to present
    RecordCommandBuffer(image_index);
    VkCommandBuffer command_buffer = CommandBuffers[image_index];
    
    // End the frame
    {
        vk::EndFrame(image_index, &command_buffer, 1);
        
        if (khr_result == VK_ERROR_OUT_OF_DATE_KHR ||
            khr_result == VK_SUBOPTIMAL_KHR || GameNeedsResize )
        {
            u32 width, height;
            PlatformGetClientWindowDimensions(&width, &height);
            
            WindowResizeEvent event;
            event.Width  = width;
            event.Height = height;
            
            event::Dispatch<WindowResizeEvent>(event);
        }
        else if (khr_result != VK_SUCCESS) {
            mprinte("Something went wrong acquiring the swapchain image!\n");
        }
    }
#endif
}

file_internal void RecordCommandBuffer(u32 idx)
{
#if 0
    VkCommandBuffer command_buffer = CommandBuffers[idx];
    VkFramebuffer framebuffer = Framebuffer[idx];
    
    vk::BeginCommandBuffer(command_buffer);
    
    
    // Bind global descriptor
    VkDescriptorSet frame_ds = GlobalDescriptorSets[i];
    vk::BindDescriptorSets(command_buffer, PipelineLayout,
                           0, 1,
                           &frame_ds,
                           0, 0);
    
    VkExtent2D extent = vk::GetSwapChainExtent();
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = (float) extent.width;
    viewport.height = (float) extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    
    vk::SetViewport(command_buffer, 0, 1, &viewport);
    vk::SetScissor(command_buffer, 0, 1, &scissor);
    
    VkClearValue clear_values[1] = {};
    clear_values[0].color = {0.67f, 0.85f, 0.90f, 1.0f};
    
    vk::BeginRenderPass(command_buffer, clear_values, 2, framebuffer, RenderPass);
    {
        // Issue draw commands...
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
    }
    vk::EndRenderPass(command_buffer);
    
    vk::EndCommandBuffer(command_buffer);
#endif
}

void GameShutdown()
{
    vk::Idle();
    ImGui_ImplVulkan_Shutdown();
    
#if 0
    u32 swapchain_image_count = vk::GetSwapChainImageCount();
    
    //vk::DestroyVmaBuffer(VertexBuffer.Handle, VertexBuffer.Memory);
    
    for (u32 i = 0; i < swapchain_image_count; ++i)
        vk::DestroyVmaBuffer(MvpUniforms[i].Buffer.Handle, MvpUniforms[i].Buffer.Memory);
    pfree(MvpUniforms);
    
    vk::DestroyPipeline(Pipeline);
    vk::DestroyPipelineLayout(PipelineLayout);
#endif
    
    EXE_PATH.Clear();
    SHADER_PATH.Clear();
    
    VERT_SHADER.Clear();
    FRAG_SHADER.Clear();
}

void GameInit()
{
    mprint("Initializing the game...\n");
    
    //~ Vulkan Init State
    // Create the CommandPool
    // NOTE(Dustin): CommandPool Creation and management is internal to GpuStage (backend.cpp)
    // NOTE(Dustin): DescriptorPool Creation and management is internal Resource Manager (resources.cpp)
    {
    }
    
    //~ Create Vertex Buffers...
    /*
    VkBufferCreateInfo vertex_buffer_info = {};
    vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buffer_info.size = 3 * sizeof(Vertex);
    vertex_buffer_info.usage =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    VmaAllocationCreateInfo vertex_alloc_info = {};
    vertex_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    vk::CreateVmaBufferWithStaging(CommandPool,
                                   vertex_buffer_info,
                                   vertex_alloc_info,
                                   VertexBuffer.Handle,
                                   VertexBuffer.Memory,
                                   vertices,
                                   vertex_buffer_info.size);
    */
    
    
    //~ Events...
    
    // Register for WindowResize
    event::Subscribe<ResizeEvent>(&GameResize, nullptr);
    
    IsGameInit = true;
    
    //~ Asset Loading
    
    jstring ExampleModel = InitJString("data/models/Fox/glTF/fox.gltf");
    ModelAsset = masset::LoadModel(ExampleModel, CommandPool);
    
    ExampleModel.Clear();
}