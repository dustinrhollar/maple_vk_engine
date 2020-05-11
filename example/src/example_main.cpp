
// Shader representation of a Vertex.
struct Vertex
{
    Vec3 Position;
    
    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        return bindingDescription;
    }
    
    static JTuple<VkVertexInputAttributeDescription*, int> GetAttributeDescriptions() {
        // Create Attribute Descriptions
        VkVertexInputAttributeDescription *attribs = talloc<VkVertexInputAttributeDescription>(1);
        
        // Positions
        attribs[0].binding = 0;
        attribs[0].location = 0;
        attribs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribs[0].offset = offsetof(Vertex, Position);
        
        JTuple<VkVertexInputAttributeDescription*, int> tuple(attribs, 1);
        return tuple;
    }
};


Vec3 vertices[] = {
    {-0.5f, -0.5f, 0.0f},
    {0.5f, -0.5f, 0.0f},
    {0.0f,  0.5f, 0.0f}
};

file_global VkRenderPass     RenderPass;
file_global VkCommandPool    CommandPool;

file_global VkFramebuffer   *Framebuffer;
file_global u32              FramebufferCount;

file_global VkDescriptorPoolSize DescriptorSizes[2];
file_global VkDescriptorPool     DescriptorPool;

file_global VkCommandBuffer *CommandBuffers;
file_global u32              CommandBuffersCount;

file_global VkPipelineLayout PipelineLayout;
file_global VkPipeline       Pipeline;

file_global BufferParameters VertexBuffer;

file_global bool IsGameInit      = false;
file_global bool GameNeedsResize = false;

file_global jstring EXE_PATH;
file_global jstring SHADER_PATH;

file_global jstring VERT_SHADER;
file_global jstring FRAG_SHADER;

file_internal void RecordCommandBuffer(u32 idx);
file_internal void CreateVulkanResizableState();

file_internal void CreateVulkanResizableState()
{
    u32 swapchain_image_count = vk::GetSwapChainImageCount();
    VkFormat swapchain_image_format = vk::GetSwapChainImageFormat();
    
    // Create the RenderPass
    //---------------------------------------------------------------------------
    {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format         = swapchain_image_format;
        colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        
        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription subpass    = {};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &colorAttachmentRef;
        
        VkAttachmentDescription attachments[1] = {
            colorAttachment,
        };
        
        RenderPass = vk::CreateRenderPass(attachments, 1,
                                          &subpass, 1,
                                          nullptr, 0);
    }
    
    // Create the Framebuffer and Depth Resources
    {
        FramebufferCount = swapchain_image_count;
        Framebuffer = palloc<VkFramebuffer>(swapchain_image_count);
        ImageParameters* swapchain_images = vk::GetSwapChainImages();
        for (u32 i = 0; i < swapchain_image_count; ++i) {
            
            VkImageView attachments[] = {
                swapchain_images[i].View,
            };
            
            VkFramebuffer framebuffer = vk::CreateFramebuffer(attachments, 1, i, RenderPass);
            Framebuffer[i] = framebuffer;
        }
    }
    
    // Descriptor Pool
    {
        DescriptorSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        DescriptorSizes[0].descriptorCount = swapchain_image_count;
        
        DescriptorSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        DescriptorSizes[1].descriptorCount = swapchain_image_count;
        
        DescriptorPool = vk::CreateDescriptorPool(DescriptorSizes, 2, 10 * swapchain_image_count);
    }
    
    // Create Command buffers
    {
        CommandBuffers = palloc<VkCommandBuffer>(swapchain_image_count);
        
        CommandBuffersCount = swapchain_image_count;
        vk::CreateCommandBuffers(CommandPool,
                                 VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                 swapchain_image_count,
                                 CommandBuffers);
    }
}

void GameResize()
{
    if (!IsGameInit) return;
    
    u32 width, height;
    PlatformGetClientWindowDimensions(&width, &height);
    
    // Idle <- wait for last frame to finish rendering
    vk::Idle();
    
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
    
    vk::Resize();
    
    CreateVulkanResizableState();
}

void FlagGameResize()
{
    GameNeedsResize = true;
}

void GameUpdateAndRender(FrameInput input)
{
    mm::ResetTransientMemory();
    
    u32 image_index;
    
    VkResult khr_result = vk::BeginFrame(image_index);
    if (khr_result == VK_ERROR_OUT_OF_DATE_KHR) {
        GameResize();
        return;
    } else if (khr_result != VK_SUCCESS &&
               khr_result != VK_SUBOPTIMAL_KHR) {
        mprinte("Failed to acquire swap chain image!");
    }
    
    // Command buffer to present
    RecordCommandBuffer(image_index);
    VkCommandBuffer command_buffer = CommandBuffers[image_index];
    
    // End the frame
    {
        vk::EndFrame(image_index, &command_buffer, 1);
        
        if (khr_result == VK_ERROR_OUT_OF_DATE_KHR ||
            khr_result == VK_SUBOPTIMAL_KHR || GameNeedsResize ) {
            
            GameResize();
            GameNeedsResize = false;
        }
        else if (khr_result != VK_SUCCESS) {
            mprinte("Something went wrong acquiring the swapchain image!\n");
        }
    }
}

file_internal void RecordCommandBuffer(u32 idx)
{
    VkCommandBuffer command_buffer = CommandBuffers[idx];
    VkFramebuffer framebuffer = Framebuffer[idx];
    
    vk::BeginCommandBuffer(command_buffer);
    
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
        vk::BindPipeline(command_buffer, Pipeline);
        
        VkBuffer vertex_buffers[] = {VertexBuffer.Handle};
        VkDeviceSize offsets[] = {0};
        vk::BindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
        
        vk::Draw(command_buffer, 3, 1, 0, 0);
        
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
}

void GameShutdown()
{
    vk::Idle();
    
    ImGui_ImplVulkan_Shutdown();
    
    vk::DestroyDescriptorPool(DescriptorPool);
    
    vk::DestroyVmaBuffer(VertexBuffer.Handle, VertexBuffer.Memory);
    
    vk::DestroyCommandBuffers(CommandPool, CommandBuffersCount,
                              CommandBuffers);
    pfree(CommandBuffers);
    
    vk::DestroyPipeline(Pipeline);
    vk::DestroyPipelineLayout(PipelineLayout);
    
    // Destroy framebuffer
    for (u32 i = 0; i < FramebufferCount; ++i)
    {
        vk::DestroyFramebuffer(Framebuffer[i]);
    }
    pfree(Framebuffer);
    
    vk::DestroyCommandPool(CommandPool);
    
    vk::DestroyRenderPass(RenderPass);
    
    EXE_PATH.Clear();
    SHADER_PATH.Clear();
    
    VERT_SHADER.Clear();
    FRAG_SHADER.Clear();
}

void GameInit()
{
    mprint("Initializing the game...\n");
    
    VERT_SHADER = InitJString("data/shaders/shader.vert.spv");
    FRAG_SHADER = InitJString("data/shaders/shader.frag.spv");
    
    //~ Vulkan Init State
    // Create the CommandPool
    {
        CommandPool = vk::CreateCommandPool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    }
    
    CreateVulkanResizableState();
    
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
        
        /*
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::ShowDemoWindow();
            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
        }
        ImGui::EndFrame();
        */
    }
    vk::EndSingleTimeCommands(command_buffer, CommandPool);
    
    //~ Create the shader...
    jstring vert_shader = PlatformLoadFile(VERT_SHADER);
    jstring frag_shader = PlatformLoadFile(FRAG_SHADER);
    
    VkShaderModule vshad_module = vk::CreateShaderModule(vert_shader.GetCStr(), vert_shader.len);
    VkShaderModule fshad_module = vk::CreateShaderModule(frag_shader.GetCStr(), frag_shader.len);
    
    // Shader pipeline
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vshad_module;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fshad_module;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };
    
    // Create Vertex Descriptions: interleaved format
    VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();
    JTuple<VkVertexInputAttributeDescription*, int> attribs = Vertex::GetAttributeDescriptions();
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attribs.Second();
    vertexInputInfo.pVertexAttributeDescriptions    = attribs.First();
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
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
    
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
    
    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading      = 1.0f; // Optional
    multisampling.pSampleMask           = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable      = VK_FALSE; // Optional
    
    // Depth/Stencil Testing - not right now
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional
    
    // Color Blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    
    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    PipelineLayout = vk::CreatePipelineLayout(pipelineLayoutInfo);
    
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.flags = 0;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_states;
    
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamic_state_info; // Optional
    pipelineInfo.layout = PipelineLayout;
    pipelineInfo.renderPass = RenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional
    
    Pipeline = vk::CreatePipeline(pipelineInfo);
    
    vk::DestroyShaderModule(vshad_module);
    vk::DestroyShaderModule(fshad_module);
    
    vert_shader.Clear();
    frag_shader.Clear();
    
    //~ Create Vertex Buffers...
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
    
    //RecordCommandBuffer();
    
    IsGameInit = true;
}