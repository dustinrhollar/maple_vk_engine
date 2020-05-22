
file_global VkRenderPass     RenderPass;
file_global VkCommandPool    CommandPool;

file_global VkFramebuffer   *Framebuffers;
file_global u32              FramebufferCount;
file_global ImageParameters  DepthResources;

file_global VkCommandBuffer *CommandBuffers;
file_global u32              CommandBuffersCount;

void GpuStageInit(frame_params *FrameParams)
{
    u32 swapchain_image_count       = vk::GetSwapChainImageCount();
    VkFormat swapchain_image_format = vk::GetSwapChainImageFormat();
    VkExtent2D extent               = vk::GetSwapChainExtent();
    
    CommandPool = vk::CreateCommandPool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    
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
        
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format         = vk::FindDepthFormat();
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
        
        RenderPass = vk::CreateRenderPass(attachments, 2,
                                          &subpass, 1,
                                          &dependency, 1);
    }
    
    // Create the Framebuffer and Depth Resources
    {
        VkFormat depth_format = vk::FindDepthFormat();
        if (depth_format == VK_FORMAT_UNDEFINED)
        {
            printf("Failed to find supported format!\n");
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
        
        vk::CreateVmaImage(imageInfo,
                           alloc_info,
                           DepthResources.Handle,
                           DepthResources.Memory,
                           DepthResources.AllocationInfo);
        
        if (DepthResources.Handle == VK_NULL_HANDLE) {
            printf("Error creating depth image!\n");
            return;
        }
        
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = DepthResources.Handle;
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = depth_format;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;
        
        DepthResources.View =  vk::CreateImageView(viewInfo);
        
        vk::TransitionImageLayout(CommandPool,
                                  DepthResources.Handle, depth_format,
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
        
        FramebufferCount = swapchain_image_count;
        Framebuffers = palloc<VkFramebuffer>(swapchain_image_count);
        ImageParameters* swapchain_images = vk::GetSwapChainImages();
        for (u32 i = 0; i < swapchain_image_count; ++i) {
            VkImageView attachments[] = {
                swapchain_images[i].View,
                DepthResources.View,
            };
            
            VkFramebuffer framebuffer = vk::CreateFramebuffer(attachments, 2, i, RenderPass);
            Framebuffers[i] = framebuffer;
        }
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

void GpuStageShutdown(frame_params* FrameParams)
{
    vk::Idle();
    
    vk::DestroyImageView(DepthResources.View);
    vk::DestroyVmaImage(DepthResources.Handle, DepthResources.Memory);
    
    vk::DestroyCommandBuffers(CommandPool, CommandBuffersCount, CommandBuffers);
    pfree(CommandBuffers);
    
    // Destroy framebuffer
    for (u32 i = 0; i < FramebufferCount; ++i)
    {
        vk::DestroyFramebuffer(Framebuffers[i]);
    }
    pfree(Framebuffers);
    
    vk::DestroyCommandPool(CommandPool);
    vk::DestroyRenderPass(RenderPass);
    
    FramebufferCount = 0;
    CommandBuffersCount = 0;
}


void GpuStageEntry(frame_params *FrameParams)
{
    // NOTE(Dustin): Since multithreading is not in effect, I am going to
    // create this loop with a single threaded mindset. First, Entry will
    // init the frame, command buffer, and render pass. It might be a good
    // to have another stage init Render Passes for greater control, but that
    // is not needed right now. CommandBuffer begin and end for this stage is
    // for the main command buffer. Not sure how to handle command buffer
    // generation across multiple threads quite yet (espicially for binding
    // resources).
    
    // Frame state that might be needed across commands
    u32 FrameImageIndex;
    VkCommandBuffer CommandBuffer;
    VkFramebuffer Framebuffer;
    VkResult khr_result;
    
    for (u32 Cmd = 0; Cmd < FrameParams->GpuCommandsCount; ++Cmd)
    {
        gpu_command GpuCmd = FrameParams->GpuCommands[Cmd];
        
        switch (GpuCmd.Type)
        {
            case GpuCommand_BeginFrame:
            {
                gpu_begin_frame_info *Info = static_cast<gpu_begin_frame_info*>(GpuCmd.Data);
                
                khr_result = vk::BeginFrame(FrameImageIndex);
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
                
                CommandBuffer = CommandBuffers[FrameImageIndex];
                Framebuffer = Framebuffers[FrameImageIndex];
                
                vk::BeginCommandBuffer(CommandBuffer);
                
                if (Info->HasDepth)
                {
                    VkClearValue clear_values[2] = {};
                    clear_values[0].color        = Info->Color;
                    clear_values[1].depthStencil = {Info->Depth, Info->Stencil};
                    vk::BeginRenderPass(CommandBuffer, clear_values, 2, Framebuffer, RenderPass);
                }
                else
                {
                    VkClearValue clear_values[1] = {};
                    clear_values[0].color        = Info->Color;
                    vk::BeginRenderPass(CommandBuffer, clear_values, 1, Framebuffer, RenderPass);
                }
            } break;
            
            case GpuCommand_EndFrame:
            {
                gpu_end_frame_info *Info = static_cast<gpu_end_frame_info*>(GpuCmd.Data);
                
                vk::EndRenderPass(CommandBuffer);
                vk::EndCommandBuffer(CommandBuffer);
                
                vk::EndFrame(FrameImageIndex, &CommandBuffer, 1);
                
                if (khr_result == VK_ERROR_OUT_OF_DATE_KHR ||
                    khr_result == VK_SUBOPTIMAL_KHR)
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
            } break;
            
            case GpuCommand_SetScissor:
            {
                gpu_set_scissor_info *Info = static_cast<gpu_set_scissor_info*>(GpuCmd.Data);
                
                VkRect2D scissor = {};
                scissor.offset   = {Info->XOffset, Info->YOffset};
                scissor.extent   = Info->Extent;
                
                vk::SetScissor(CommandBuffer, 0, 1, &scissor);
            } break;
            
            case GpuCommand_SetViewport:
            {
                gpu_set_viewport_info *Info = static_cast<gpu_set_viewport_info*>(GpuCmd.Data);
                
                VkViewport viewport = {};
                viewport.x          = Info->X;
                viewport.y          = Info->Y;
                viewport.width      = Info->Width;
                viewport.height     = Info->Height;
                viewport.minDepth   = 0.0f;
                viewport.maxDepth   = 1.0f;
                
                vk::SetViewport(CommandBuffer, 0, 1, &viewport);
            } break;
            
            // Upload Resources
            case GpuCommand_UploadVertexBuffer:
            {
                gpu_vertex_buffer_create_info *Info = static_cast<gpu_vertex_buffer_create_info*>(GpuCmd.Data);
                vk::CreateVmaBufferWithStaging(CommandPool,
                                               Info->BufferCreateInfo,
                                               Info->VmaCreateInfo,
                                               *(Info->Buffer),
                                               *(Info->Allocation),
                                               Info->Data,
                                               Info->Size);
            } break;
            
            case GpuCommand_UploadIndexBuffer:
            {
                gpu_index_buffer_create_info *Info = static_cast<gpu_index_buffer_create_info*>(GpuCmd.Data);
                vk::CreateVmaBufferWithStaging(CommandPool,
                                               Info->BufferCreateInfo,
                                               Info->VmaCreateInfo,
                                               *(Info->Buffer),
                                               *(Info->Allocation),
                                               Info->Data,
                                               Info->Size);
            } break;
            
            case GpuCommand_UploadImage:
            {
            } break;
            
            case GpuCommand_Draw:
            {
            } break;
            
            case GpuCommand_DrawIndexed:
            {
            } break;
            
            case GpuCommand_CreateDescriptor:
            {
            } break;
            
            case GpuCommand_UpdateDescriptor:
            {
            } break;
            
            case GpuCommand_CopyDescriptor:
            {
            } break;
            
            case GpuCommand_BindDescriptor:
            {
            } break;
            
            case GpuCommand_BeginCommandBuffer:
            {
            } break;
            
            case GpuCommand_EndCommandBuffer:
            
            // Render Pass
            case GpuCommand_BeginRenderPass:
            {
            } break;
            
            case GpuCommand_EndRenderPass:
            {
            } break;
            
            default:
            {
                mprinte("Unknown Gpu Command %d!\n", GpuCmd.Type);
                break;
            }
        }
    }
}

VkRenderPass GetPrimaryRenderPass()
{
    return RenderPass;
}
