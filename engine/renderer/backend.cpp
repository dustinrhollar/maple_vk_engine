

file_global VkRenderPass     RenderPass;
file_global VkCommandPool    CommandPool;

file_global VkFramebuffer   *Framebuffer;
file_global u32              FramebufferCount;
file_global ImageParameters  DepthResources;

file_global VkCommandBuffer *CommandBuffers;
file_global u32              CommandBuffersCount;


void GpuStageInit(frame_params FrameParams)
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
        Framebuffer = palloc<VkFramebuffer>(swapchain_image_count);
        ImageParameters* swapchain_images = vk::GetSwapChainImages();
        for (u32 i = 0; i < swapchain_image_count; ++i) {
            VkImageView attachments[] = {
                swapchain_images[i].View,
                DepthResources.View,
            };
            
            VkFramebuffer framebuffer = vk::CreateFramebuffer(attachments, 2, i, RenderPass);
            Framebuffer[i] = framebuffer;
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


void GpuStageEntry(frame_params FrameParams)
{
}
