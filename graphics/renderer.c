
void renderer_init(renderer *Renderer)
{
    Renderer->RenderMode = RenderMode_Solid;
    
    u32 swapchain_image_count       = Core->VkCore.GetSwapChainImageCount();
    VkFormat swapchain_image_format = Core->VkCore.GetSwapChainImageFormat();
    VkExtent2D extent               = Core->VkCore.GetSwapChainExtent();
    VkFormat depth_format           = Core->VkCore.FindDepthFormat();
    
    // Create the command pool
    Renderer->CommandPool = Core->VkCore.CreateCommandPool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                                           VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    
    // Init renderpass
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
        
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format         = depth_format;
        depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        VkSubpassDependency dependency{};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        
        VkSubpassDescription subpass    = {};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        
        VkAttachmentDescription attachments[2] = {
            colorAttachment,
            depthAttachment,
        };
        
        Renderer->PrimaryRenderPass = Core->VkCore.CreateRenderPass(attachments, 2,
                                                                    &subpass, 1,
                                                                    &dependency, 1);
    }
    
    // Framebuffer + Depth buffer
    {
        if (depth_format == VK_FORMAT_UNDEFINED)
        {
            mprinte("Failed to find supported format!\n");
            return;
        }
        
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = (u32)(extent.width);
        imageInfo.extent.height = (u32)(extent.height);
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = depth_format;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags         = 0; // Optional
        
        VmaAllocationCreateInfo alloc_info = {};
        alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        Core->VkCore.CreateVmaImage(imageInfo,
                                    alloc_info,
                                    Renderer->DepthResources.Handle,
                                    Renderer->DepthResources.Memory,
                                    Renderer->DepthResources.AllocationInfo);
        
        if (Renderer->DepthResources.Handle == VK_NULL_HANDLE) {
            printf("Error creating depth image!\n");
            return;
        }
        
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = Renderer->DepthResources.Handle;
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = depth_format;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;
        
        Renderer->DepthResources.View =  Core->VkCore.CreateImageView(viewInfo);
        
        Core->VkCore.TransitionImageLayout(Renderer->CommandPool,
                                           Renderer->DepthResources.Handle, depth_format,
                                           VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
        
        Renderer->FramebufferCount = swapchain_image_count;
        Renderer->Framebuffers = palloc<VkFramebuffer>(swapchain_image_count);
        image_parameters* swapchain_images = Core->VkCore.GetSwapChainImages();
        for (u32 i = 0; i < swapchain_image_count; ++i) {
            VkImageView attachments[] = {
                swapchain_images[i].View,
                Renderer->DepthResources.View,
            };
            
            VkFramebuffer framebuffer = Core->VkCore.CreateFramebuffer(attachments, 2, i, Renderer->PrimaryRenderPass);
            Renderer->Framebuffers[i] = framebuffer;
        }
    }
    
    // Init command pool & buffers
    {
        Renderer->CommandBuffers = palloc<VkCommandBuffer>(swapchain_image_count);
        
        Renderer->CommandBuffersCount = swapchain_image_count;
        Core->VkCore.CreateCommandBuffers(Renderer->CommandPool,
                                          VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                          swapchain_image_count,
                                          Renderer->CommandBuffers);
    }
    
    
    // Create the Descriptor Pool
    {
        u32 SwapChainImageCount = Core->VkCore.GetSwapChainImageCount();
        
        const u32 SizeCount = 3;
        u32 MaxSets = SizeCount * SwapChainImageCount;
        
        VkDescriptorPoolSize DescriptorPoolSizes[SizeCount];
        
        DescriptorPoolSizes[0] = {0};
        DescriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        DescriptorPoolSizes[0].descriptorCount = SwapChainImageCount;
        
        DescriptorPoolSizes[1] = {0};
        DescriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        DescriptorPoolSizes[1].descriptorCount = SwapChainImageCount;
        
        DescriptorPoolSizes[2] = {0};
        DescriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        DescriptorPoolSizes[2].descriptorCount = SwapChainImageCount;
        
        Renderer->DescriptorPool = Core->VkCore.CreateDescriptorPool(DescriptorPoolSizes, 
                                                                     SizeCount, 
                                                                     MaxSets,
                                                                     0);
    }
    global_shader_data_init(&Renderer->GlobalShaderData);
    object_data_buffer_init(&Renderer->ObjectDataBuffer);
    
    Renderer->CurrentImageIndex = 0;
}

void renderer_free(renderer *Renderer)
{
    Core->VkCore.Idle();
    
    object_data_buffer_free(&Renderer->ObjectDataBuffer);
    global_shader_data_free(&Renderer->GlobalShaderData);
    Core->VkCore.DestroyDescriptorPool(Renderer->DescriptorPool);
    
    Core->VkCore.DestroyImageView(Renderer->DepthResources.View);
    Core->VkCore.DestroyVmaImage(Renderer->DepthResources.Handle, 
                                 Renderer->DepthResources.Memory);
    
    Core->VkCore.DestroyCommandBuffers(Renderer->CommandPool, 
                                       Renderer->CommandBuffersCount, 
                                       Renderer->CommandBuffers);
    pfree(Renderer->CommandBuffers);
    
    // Destroy framebuffer
    for (u32 i = 0; i < Renderer->FramebufferCount; ++i)
    {
        Core->VkCore.DestroyFramebuffer(Renderer->Framebuffers[i]);
    }
    pfree(Renderer->Framebuffers);
    
    Core->VkCore.DestroyCommandPool(Renderer->CommandPool);
    Core->VkCore.DestroyRenderPass(Renderer->PrimaryRenderPass);
    
    Renderer->FramebufferCount = 0;
    Renderer->CommandBuffersCount = 0;
}

u32 renderer_begin_frame()
{
    u32 Result;
    
    { // Begin the frame
        Core->VkCore.BeginFrame(Result);
        
        // TODO(Dustin): Handle Resize
        
        
        Core->Renderer->CurrentImageIndex = Result;
        Core->Renderer->ActiveCommandBuffer  = Core->Renderer->CommandBuffers + Result;
        Core->VkCore.BeginCommandBuffer(*Core->Renderer->ActiveCommandBuffer);
    }
    
    // Setup render state
    {
        VkCommandBuffer *ActiveCommandBuffer = Core->Renderer->ActiveCommandBuffer;
        VkFramebuffer Framebuffer = Core->Renderer->Framebuffers[Result];
        
        VkClearColorValue ClearValue = { 0.67f, 0.85f, 0.90f, 1.0f };
        
        VkClearValue clear_values[2] = {};
        clear_values[0].color        = ClearValue;
        clear_values[1].depthStencil = { 1.0f, 0 };
        Core->VkCore.BeginRenderPass(*ActiveCommandBuffer, clear_values, 2, Framebuffer, Core->Renderer->PrimaryRenderPass);
        
        //render_set_viewport_info *ViewportInfo = talloc<render_set_viewport_info>(1);
        VkExtent2D Extent = Core->VkCore.GetSwapChainExtent();
        
        u32 Width, Height;
        PlatformGetClientWindowDimensions(&Width, &Height);
        
        VkRect2D Scissor = {};
        Scissor.offset   = {0, 0};
        Scissor.extent   = Extent;
        Core->VkCore.SetScissor(*ActiveCommandBuffer, 0, 1, &Scissor);
        
        VkViewport Viewport = {};
        Viewport.x          = 0;
        Viewport.y          = 0;
        Viewport.width      = Width;
        Viewport.height     = Height;
        Viewport.minDepth   = 0.0f;
        Viewport.maxDepth   = 1.0f;
        Core->VkCore.SetViewport(*ActiveCommandBuffer, 0, 1, &Viewport);
    }
    
    return Result; 
}

void renderer_end_frame()
{
    Core->VkCore.EndRenderPass(*Core->Renderer->ActiveCommandBuffer);
    Core->VkCore.EndCommandBuffer(*Core->Renderer->ActiveCommandBuffer);
    Core->VkCore.EndFrame(Core->Renderer->CurrentImageIndex, 
                          Core->Renderer->ActiveCommandBuffer, 1);
    
    // TODO(Dustin): Handle Resize 
#if 0
    if (khr_result == VK_ERROR_OUT_OF_DATE_KHR ||
        khr_result == VK_SUBOPTIMAL_KHR)
    {
        u32 width, height;
        PlatformGetClientWindowDimensions(&width, &height);
        
    }
    else if (khr_result != VK_SUCCESS) {
        mprinte("Something went wrong acquiring the swapchain image!\n");
    }
#endif
    
    Core->Renderer->ActiveCommandBuffer = NULL;
    object_data_buffer_end_frame(&Core->Renderer->ObjectDataBuffer);
}


void object_data_buffer_init(object_data_buffer *ObjectDataBuffer)
{
    u32 SwapChainImageCount = Core->VkCore.GetSwapChainImageCount();
    
    // Create the Descriptor Layout
    VkDescriptorSetLayoutBinding Bindings[1]= {};
    Bindings[0].binding            = 0;
    Bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    Bindings[0].descriptorCount    = 1;
    Bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    Bindings[0].pImmutableSamplers = nullptr;
    
    ObjectDataBuffer->DescriptorLayout = Core->VkCore.CreateDescriptorSetLayout(Bindings, 1);
    
    // Create the Descriptor Sets
    ObjectDataBuffer->DescriptorSets = (VkDescriptorSet*)memory_alloc(Core->Memory, 
                                                                      sizeof(VkDescriptorSet) * SwapChainImageCount);
    ObjectDataBuffer->DescriptorSetsCount = SwapChainImageCount;
    
    VkDescriptorSetLayout *Layouts = (VkDescriptorSetLayout*)memory_alloc(Core->Memory, 
                                                                          sizeof(VkDescriptorSetLayout) * SwapChainImageCount);
    for (u32 LayoutIdx = 0; LayoutIdx < SwapChainImageCount; ++LayoutIdx)
        Layouts[LayoutIdx] = ObjectDataBuffer->DescriptorLayout;
    
    VkDescriptorSetAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    AllocInfo.descriptorPool     = Core->Renderer->DescriptorPool;
    AllocInfo.descriptorSetCount = SwapChainImageCount;
    AllocInfo.pSetLayouts        = Layouts;
    
    Core->VkCore.CreateDescriptorSets(ObjectDataBuffer->DescriptorSets,
                                      AllocInfo);
    
    memory_release(Core->Memory, Layouts);
    
    // HACK(Dustin): HARDCODING THE SIZE OF THE BUFFER. PROBABLY WANT
    // TO ALLOW FOR THE PLATFORM TO SET THIS.
    
    u32 NumObjectsPerFrame = 20;
    u32 SizePerObject = sizeof(object_data);
    mp_dynamic_uniform_buffer_init(&ObjectDataBuffer->Buffer, NumObjectsPerFrame * SizePerObject);
    
    for (u32 i = 0; i < SwapChainImageCount; ++i) 
    {
        VkWriteDescriptorSet DescriptorWrites[1] = {};
        
        VkDescriptorBufferInfo FrameBufferInfo = {};
        FrameBufferInfo.buffer                 = ObjectDataBuffer->Buffer.Handles[i].Handle;
        FrameBufferInfo.offset                 = 0;
        FrameBufferInfo.range                  = VK_WHOLE_SIZE;
        
        DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[0].dstSet           = ObjectDataBuffer->DescriptorSets[i];
        DescriptorWrites[0].dstBinding       = 0;
        DescriptorWrites[0].dstArrayElement  = 0;
        DescriptorWrites[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        DescriptorWrites[0].descriptorCount  = 1;
        DescriptorWrites[0].pBufferInfo      = &FrameBufferInfo;
        DescriptorWrites[0].pImageInfo       = nullptr; // Optional
        DescriptorWrites[0].pTexelBufferView = nullptr; // Optional
        
        Core->VkCore.UpdateDescriptorSets(DescriptorWrites, 1);
    }
}

void object_data_buffer_free(object_data_buffer *ObjectData)
{
    Core->VkCore.DestroyDescriptorSetLayout(ObjectData->DescriptorLayout);
    memory_release(Core->Memory, ObjectData->DescriptorSets);
    ObjectData->DescriptorSetsCount = 0;
    
    mp_dynamic_uniform_buffer_free(&ObjectData->Buffer);
}

void object_data_buffer_begin_frame(object_data_buffer *ObjectDataBuffer)
{
}

void object_data_buffer_end_frame(object_data_buffer *ObjectData)
{
    mp_dynamic_uniform_buffer_reset(&ObjectData->Buffer);
}

void object_data_buffer_update(object_data_buffer *ObjectData)
{
}


void global_shader_data_init(global_shader_data *ShaderData)
{
    u32 SwapChainImageCount = Core->VkCore.GetSwapChainImageCount();
    
    // Create the Descriptor Layout
    VkDescriptorSetLayoutBinding Bindings[1]= {};
    Bindings[0].binding            = 0;
    Bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    Bindings[0].descriptorCount    = 1;
    Bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
    //Bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    Bindings[0].pImmutableSamplers = nullptr;
    
    ShaderData->DescriptorLayout = Core->VkCore.CreateDescriptorSetLayout(Bindings, 1);
    
    // Create the Descriptor Sets
    ShaderData->DescriptorSets = (VkDescriptorSet*)memory_alloc(Core->Memory, 
                                                                sizeof(VkDescriptorSet) * SwapChainImageCount);
    ShaderData->DescriptorSetsCount = SwapChainImageCount;
    
    VkDescriptorSetLayout *Layouts = (VkDescriptorSetLayout*)memory_alloc(Core->Memory, 
                                                                          sizeof(VkDescriptorSetLayout) * SwapChainImageCount);
    for (u32 LayoutIdx = 0; LayoutIdx < SwapChainImageCount; ++LayoutIdx)
        Layouts[LayoutIdx] = ShaderData->DescriptorLayout;
    
    VkDescriptorSetAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    AllocInfo.descriptorPool     = Core->Renderer->DescriptorPool;
    AllocInfo.descriptorSetCount = SwapChainImageCount;
    AllocInfo.pSetLayouts        = Layouts;
    
    Core->VkCore.CreateDescriptorSets(ShaderData->DescriptorSets,
                                      AllocInfo);
    
    memory_release(Core->Memory, Layouts);
    
    mp_uniform_buffer_init(&ShaderData->Buffer, sizeof(camera_data));
    
    for (u32 i = 0; i < SwapChainImageCount; ++i) 
    {
        VkWriteDescriptorSet DescriptorWrites[1] = {};
        
        VkDescriptorBufferInfo FrameBufferInfo = {};
        FrameBufferInfo.buffer                 = ShaderData->Buffer.Handles[i].Handle;
        FrameBufferInfo.offset                 = 0;
        FrameBufferInfo.range                  = VK_WHOLE_SIZE;
        
        DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[0].dstSet           = ShaderData->DescriptorSets[i];
        DescriptorWrites[0].dstBinding       = 0;
        DescriptorWrites[0].dstArrayElement  = 0;
        DescriptorWrites[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        DescriptorWrites[0].descriptorCount  = 1;
        DescriptorWrites[0].pBufferInfo      = &FrameBufferInfo;
        DescriptorWrites[0].pImageInfo       = nullptr; // Optional
        DescriptorWrites[0].pTexelBufferView = nullptr; // Optional
        
        Core->VkCore.UpdateDescriptorSets(DescriptorWrites, 1);
    }
}

void global_shader_data_free(global_shader_data *ShaderData)
{
    Core->VkCore.DestroyDescriptorSetLayout(ShaderData->DescriptorLayout);
    memory_release(Core->Memory, ShaderData->DescriptorSets);
    ShaderData->DescriptorSetsCount = 0;
    
    mp_uniform_buffer_free(&ShaderData->Buffer);
}

void global_shader_data_update(global_shader_data *ShaderData)
{
    
}
