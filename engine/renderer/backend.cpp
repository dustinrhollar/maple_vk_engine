
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
    
    // Gpu Entry can be called when there is not an actually frame to submit,
    // so this flag is used for when a begin frame is detected in the GpuCommands
    // list.
    bool FrameActive = false;
    
    for (u32 Cmd = 0; Cmd < FrameParams->GpuCommandsCount; ++Cmd)
    {
        gpu_command GpuCmd = FrameParams->GpuCommands[Cmd];
        
        switch (GpuCmd.Type)
        {
            case GpuCmd_BeginFrame:
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
                
                // Upload the GlobalShaderData to its corresponding Uniform for this frame...
                global_shader_data GlobalShaderData;
                mresource::UpdateGlobalFrameData(&Info->GlobalShaderData,
                                                 FrameImageIndex);
                
                // HACK(Dustin): Get the Descriptor for the Global Frame Shader Data
#if 0
                resource DefaultPipelineResource = mresource::GetDefaultPipeline();
                resource GlobalFrameDescriptor   = mresource::GetDefaultFrameDescriptor();
                VkDescriptorSet DescriptorSet    = GlobalFrameDescriptor.DescriptorSet.DescriptorSets[FrameImageIndex].Handle;
                
                vk::BindDescriptorSets(CommandBuffer, DefaultPipelineResource.Pipeline.Layout,
                                       GLOBAL_SET, 1,
                                       &DescriptorSet, 0,
                                       nullptr);
#endif
                
                FrameActive = true;
            } break;
            
            case GpuCmd_EndFrame:
            {
                gpu_end_frame_info *Info = static_cast<gpu_end_frame_info*>(GpuCmd.Data);
            } break;
            
            case GpuCmd_SetScissor:
            {
                gpu_set_scissor_info *Info = static_cast<gpu_set_scissor_info*>(GpuCmd.Data);
                
                VkRect2D scissor = {};
                scissor.offset   = {Info->XOffset, Info->YOffset};
                scissor.extent   = Info->Extent;
                
                vk::SetScissor(CommandBuffer, 0, 1, &scissor);
            } break;
            
            case GpuCmd_SetViewport:
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
            case GpuCmd_UploadVertexBuffer:
            {
                gpu_vertex_buffer_create_info *Info = static_cast<gpu_vertex_buffer_create_info*>(GpuCmd.Data);
                vk::CreateVmaBufferWithStaging(CommandPool,
                                               Info->BufferCreateInfo,
                                               Info->VmaCreateInfo,
                                               (Info->BufferParams->Handle),
                                               (Info->BufferParams->Memory),
                                               Info->Data,
                                               Info->Size);
            } break;
            
            case GpuCmd_UploadIndexBuffer:
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
            
            case GpuCmd_UploadImage:
            {
                gpu_image_create_info *Info = static_cast<gpu_image_create_info*>(GpuCmd.Data);
                
                i32 Width, Height, Channels;
                unsigned char *Data = stbi_load(Info->Filename.GetCStr(), &Width, &Height, &Channels, STBI_rgb_alpha);
                VkDeviceSize ImageSize = Width * Height * 4;
                
                if (!Data)
                {
                    mprinte("Failed to load texture from file %s!\n", Info->Filename.GetCStr());
                }
                else
                {
                    // size, usage, properties
                    VkBufferCreateInfo BufferCreateInfo = {};
                    BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    BufferCreateInfo.size        = ImageSize;
                    BufferCreateInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    // prop: VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                    VmaAllocationCreateInfo VmaCreateInfo = {};
                    VmaCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
                    
                    BufferParameters StagingBuffer;
                    
                    StagingBuffer.Size = ImageSize;
                    vk::CreateVmaBuffer(BufferCreateInfo,
                                        VmaCreateInfo,
                                        StagingBuffer.Handle,
                                        StagingBuffer.Memory,
                                        StagingBuffer.AllocationInfo);
                    
                    void *MappedMemory;
                    vk::VmaMap(&MappedMemory, StagingBuffer.Memory);
                    {
                        memcpy(MappedMemory, Data, StagingBuffer.Size);
                    }
                    vk::VmaUnmap(StagingBuffer.Memory);
                    
                    stbi_image_free(Data);
                    
                    // Calculate mip levels
#define MAX(x,y) ((x) >= (y) ? (x) : (y))
                    u32 MipLevels = ((u32)floor(log2(MAX(Width, Height)))) + 1;
#undef MAX
                    
                    // Create the Image for the component
                    VkImageCreateInfo ImageInfo = {};
                    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    ImageInfo.imageType     = VK_IMAGE_TYPE_2D;
                    ImageInfo.extent.width  = (u32)(Width);
                    ImageInfo.extent.height = (u32)(Height);
                    ImageInfo.extent.depth  = 1;
                    ImageInfo.mipLevels     = MipLevels;
                    ImageInfo.arrayLayers   = 1;
                    ImageInfo.format        = VK_FORMAT_R8G8B8A8_SRGB;
                    ImageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
                    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    ImageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                        VK_IMAGE_USAGE_SAMPLED_BIT;
                    ImageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
                    ImageInfo.samples       = VK_SAMPLE_COUNT_1_BIT; // NOTE(Dustin): Anti-Aliasing?
                    ImageInfo.flags         = 0; // Optional
                    
                    VmaAllocationCreateInfo AllocInfo = {};
                    AllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
                    
                    // Create the image and copy the data into it.
                    vk::CreateVmaImage(ImageInfo, AllocInfo,
                                       Info->Image->Handle,
                                       Info->Image->Memory,
                                       Info->Image->AllocationInfo);
                    
                    vk::TransitionImageLayout(CommandPool,
                                              Info->Image->Handle,
                                              VK_FORMAT_R8G8B8A8_SRGB,
                                              VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                              MipLevels);
                    
                    vk::CopyBufferToImage(CommandPool,
                                          StagingBuffer.Handle,
                                          Info->Image->Handle,
                                          Width, Height);
                    
                    // Image is transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    // while generating mipmaps
                    
                    vk::DestroyVmaBuffer(StagingBuffer.Handle, StagingBuffer.Memory);
                    
                    vk::GenerateMipmaps(CommandPool,
                                        Info->Image->Handle, VK_FORMAT_R8G8B8A8_SRGB,
                                        Width, Height, MipLevels);
                    
                    // Create the image view for the texture
                    VkImageViewCreateInfo ViewInfo = {};
                    ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    ViewInfo.image                           = Info->Image->Handle;
                    ViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
                    ViewInfo.format                          = VK_FORMAT_R8G8B8A8_SRGB;
                    ViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                    ViewInfo.subresourceRange.baseMipLevel   = 0;
                    ViewInfo.subresourceRange.levelCount     = MipLevels;
                    ViewInfo.subresourceRange.baseArrayLayer = 0;
                    ViewInfo.subresourceRange.layerCount     = 1;
                    
                    Info->Image->View = vk::CreateImageView(ViewInfo);
                    
                    // Create the Texture Sampler
                    VkSamplerCreateInfo SamplerInfo = {};
                    SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                    SamplerInfo.magFilter               = Info->MagFilter;
                    SamplerInfo.minFilter               = Info->MinFilter;
                    SamplerInfo.addressModeU            = Info->AddressModeU;
                    SamplerInfo.addressModeV            = Info->AddressModeV;
                    SamplerInfo.addressModeW            = Info->AddressModeW;
                    SamplerInfo.anisotropyEnable        = VK_TRUE;
                    SamplerInfo.maxAnisotropy           = 16;
                    SamplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                    SamplerInfo.unnormalizedCoordinates = VK_FALSE;
                    SamplerInfo.compareEnable           = VK_FALSE;
                    SamplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
                    SamplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                    SamplerInfo.mipLodBias              = 0.0f;
                    SamplerInfo.minLod                  = 0.0f;
                    SamplerInfo.maxLod                  = static_cast<r32>(MipLevels);
                    
                    Info->Image->Sampler = vk::CreateImageSampler(SamplerInfo);
                }
                
                Info->Filename.Clear();
            } break;
            
            case GpuCmd_Draw:
            {
                gpu_draw_info *Info = static_cast<gpu_draw_info*>(GpuCmd.Data);
                
                vk::BindVertexBuffers(CommandBuffer, 0, Info->VertexBuffersCount,
                                      Info->VertexBuffers, Info->Offsets);
                
                if (Info->IsIndexed)
                {
                    vk::BindIndexBuffer(CommandBuffer, Info->IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                    
                    vk::DrawIndexed(CommandBuffer, Info->Count,
                                    1, 0, 0, 0);
                }
                else
                {
                    vk::Draw(CommandBuffer, Info->Count, 1, 0, 0);
                }
                
            } break;
            
            case GpuCmd_CreateDescriptor:
            {
            } break;
            
            case GpuCmd_UpdateDescriptor:
            {
                gpu_descriptor_update_info *Infos =
                    static_cast<gpu_descriptor_update_info*>(GpuCmd.Data);
                
                // NOTE(Dustin): Probably do not want to force a descriptor write update
                // for all descriptors?
                u32 SwapchainImageCount = vk::GetSwapChainImageCount();
                for (u32 SwapImage = 0; SwapImage < SwapchainImageCount; ++SwapImage)
                {
                    VkWriteDescriptorSet *DescriptorWrites =
                        talloc<VkWriteDescriptorSet>(Infos->WriteInfosCount);
                    
                    for (u32 WriteInfo = 0; WriteInfo < Infos->WriteInfosCount; WriteInfo++)
                    {
                        resource DescriptorResource = mresource::GetResource(Infos->WriteInfos[WriteInfo].DescriptorId);
                        
                        DescriptorWrites[WriteInfo] = {};
                        DescriptorWrites[WriteInfo].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        DescriptorWrites[WriteInfo].dstSet           =
                            DescriptorResource.DescriptorSet.DescriptorSets[SwapImage].Handle;
                        DescriptorWrites[WriteInfo].dstBinding       =
                            Infos->WriteInfos[WriteInfo].DescriptorBinding;
                        DescriptorWrites[WriteInfo].dstArrayElement  = 0;
                        DescriptorWrites[WriteInfo].descriptorType   =
                            Infos->WriteInfos[WriteInfo].DescriptorType;
                        DescriptorWrites[WriteInfo].descriptorCount  = 1;
                        
                        if (Infos->WriteInfos[WriteInfo].DescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                            Infos->WriteInfos[WriteInfo].DescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
                        {
                            resource UniformResource = mresource::GetResource(Infos->WriteInfos[WriteInfo].BufferId);
                            
                            VkDescriptorBufferInfo *BufferInfo = talloc<VkDescriptorBufferInfo>(1);
                            *BufferInfo = {};
                            if (UniformResource.Type == Resource_UniformBuffer)
                            {
                                BufferInfo->buffer =
                                    UniformResource.UniformBuffer.Buffers[SwapImage].Handle;
                            }
                            else if (UniformResource.Type == Resource_DynamicUniformBuffer)
                            {
                                BufferInfo->buffer =
                                    UniformResource.DynamicUniformBuffer.Pools[SwapImage].Buffer.Handle;
                            }
                            else
                                mprinte("Attempting to update an incorrect buffer type with a Descriptor!\n");
                            
                            BufferInfo->offset = 0;
                            BufferInfo->range  = VK_WHOLE_SIZE;
                            
                            DescriptorWrites[WriteInfo].pBufferInfo      = BufferInfo;
                            DescriptorWrites[WriteInfo].pImageInfo       = nullptr;
                            DescriptorWrites[WriteInfo].pTexelBufferView = nullptr;
                        }
                        else if (Infos->WriteInfos[WriteInfo].DescriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                        {
                            resource ImageResource = mresource::GetResource(Infos->WriteInfos[WriteInfo].TextureId);
                            
                            VkDescriptorImageInfo *ImageInfo = talloc<VkDescriptorImageInfo>(1);
                            
                            *ImageInfo = {};
                            ImageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            ImageInfo->imageView   = ImageResource.Image.Image.View;
                            ImageInfo->sampler     = ImageResource.Image.Image.Sampler;
                            
                            DescriptorWrites[WriteInfo].pBufferInfo      = nullptr;
                            DescriptorWrites[WriteInfo].pImageInfo       = ImageInfo;
                            DescriptorWrites[WriteInfo].pTexelBufferView = nullptr;
                        }
                        else
                        {
                            mprinte("Unknown descriptor write update!\n");
                        }
                    }
                    
                    vk::UpdateDescriptorSets(DescriptorWrites, Infos->WriteInfosCount);
                }
                
            } break;
            
            case GpuCmd_UpdateBuffer:
            {
                gpu_update_buffer_info *Info = static_cast<gpu_update_buffer_info*>(GpuCmd.Data);
                mresource::UpdateUniform(Info->Uniform, Info->Data, Info->DataSize, FrameImageIndex, Info->BufferOffset);
            } break;
            
            case GpuCmd_CopyDescriptor:
            {
            } break;
            
            case GpuCmd_BindDescriptorSet:
            {
                gpu_descriptor_set_bind_info *Info = static_cast<gpu_descriptor_set_bind_info*>(GpuCmd.Data);
                
                resource Descriptor = mresource::GetResource(Info->DescriptorId);
                VkDescriptorSet DescriptorSet = Descriptor.DescriptorSet.DescriptorSets[FrameImageIndex].Handle;
                
                vk::BindDescriptorSets(CommandBuffer, Info->PipelineLayout, Info->FirstSet, 1,
                                       &DescriptorSet, Info->DynamicOffsetsCount, Info->DynamicOffsets);
            } break;
            
            case GpuCmd_BeginCommandBuffer:
            {
            } break;
            
            case GpuCmd_EndCommandBuffer:
            
            // Render Pass
            case GpuCmd_BeginRenderPass:
            {
            } break;
            
            case GpuCmd_EndRenderPass:
            {
            } break;
            
            case GpuCmd_BindPipeline:
            {
                gpu_bind_pipeline_info *Info = static_cast<gpu_bind_pipeline_info*>(GpuCmd.Data);
                
                vk::BindPipeline(CommandBuffer, Info->Pipeline);
            } break;
            
            default:
            {
                mprinte("Unknown Gpu Command %d!\n", GpuCmd.Type);
                break;
            }
        }
    }
    
    if (FrameActive)
    {
        // TODO(Dustin): End Render Pass should probably be an exposed command
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
    }
}

VkRenderPass GetPrimaryRenderPass()
{
    return RenderPass;
}
