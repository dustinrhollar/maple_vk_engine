
void MapleDevGuiInit(maple_dev_gui *DevGui, VkRenderPass RenderPass, dev_ui_draw_callback *Draw)
{
    DevGui->SwapImageCount = vk::GetSwapChainImageCount();
    DevGui->VertexBuffer   = palloc<BufferParameters>(DevGui->SwapImageCount);
    DevGui->IndexBuffer    = palloc<BufferParameters>(DevGui->SwapImageCount);
    DevGui->Draw           = Draw;
    DevGui->VertexCount    = 0;
    DevGui->IndexCount     = 0;
    
    for (u32 i = 0; i < DevGui->SwapImageCount; ++i)
    {
        DevGui->VertexBuffer[i]  = {};
        DevGui->IndexBuffer[i]   = {};
    }
    
    //~ ImGui state
    
    // TODO(Dustin): Style?
    
    
    // Dimensions
    ImGuiIO& Io = ImGui::GetIO();
    Io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for
    
    //~ Vulkan Resources
    
    DevGui->CommandPool = vk::CreateCommandPool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    
    u8* FontData;
    i32 TexWidth, TexHeight;
    Io.Fonts->GetTexDataAsRGBA32(&FontData, &TexWidth, &TexHeight);
    size_t UploadSize = TexWidth * TexHeight * 4 * sizeof(char);
    
    // Create the FontImage:
    {
        // Image
        VkImageCreateInfo Info = {};
        Info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        Info.imageType     = VK_IMAGE_TYPE_2D;
        Info.format        = VK_FORMAT_R8G8B8A8_UNORM;
        Info.extent.width  = TexWidth;
        Info.extent.height = TexHeight;
        Info.extent.depth  = 1;
        Info.mipLevels     = 1;
        Info.arrayLayers   = 1;
        Info.samples       = VK_SAMPLE_COUNT_1_BIT;
        Info.tiling        = VK_IMAGE_TILING_OPTIMAL;
        Info.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        Info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        Info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        
        VmaAllocationCreateInfo AllocInfo = {};
        AllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        vk::CreateVmaImage(Info,
                           AllocInfo,
                           DevGui->FontImage.Handle,
                           DevGui->FontImage.Memory,
                           DevGui->FontImage.AllocationInfo);
        
        // Image View
        VkImageViewCreateInfo ViewInfo = {};
        ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ViewInfo.image                           = DevGui->FontImage.Handle;
        ViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        ViewInfo.format                          = VK_FORMAT_R8G8B8A8_UNORM;
        ViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ViewInfo.subresourceRange.levelCount     = 1;
        ViewInfo.subresourceRange.layerCount     = 1;
        
        DevGui->FontImage.View = vk::CreateImageView(ViewInfo);
        
        // Staging buffers for the font image
        VkBufferCreateInfo BufferCreateInfo = {};
        BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufferCreateInfo.size        = UploadSize;
        BufferCreateInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        // prop: VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        VmaAllocationCreateInfo VmaCreateInfo = {};
        VmaCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        
        BufferParameters StagingBuffer;
        
        vk::CreateVmaBuffer(BufferCreateInfo,
                            VmaCreateInfo,
                            StagingBuffer.Handle,
                            StagingBuffer.Memory,
                            StagingBuffer.AllocationInfo);
        
        void *MappedMemory;
        vk::VmaMap(&MappedMemory, StagingBuffer.Memory);
        {
            memcpy(MappedMemory, FontData, UploadSize);
            
            vk::VmaFlushAllocation(StagingBuffer.Memory, 0, UploadSize);
        }
        vk::VmaUnmap(StagingBuffer.Memory);
        
        vk::TransitionImageLayout(DevGui->CommandPool,
                                  DevGui->FontImage.Handle,
                                  VK_FORMAT_R8G8B8A8_SRGB,
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  1);
        
        vk::CopyBufferToImage(DevGui->CommandPool,
                              StagingBuffer.Handle,
                              DevGui->FontImage.Handle,
                              TexWidth, TexHeight);
        
        vk::TransitionImageLayout(DevGui->CommandPool,
                                  DevGui->FontImage.Handle,
                                  VK_FORMAT_R8G8B8A8_SRGB,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  1);
        
        vk::DestroyVmaBuffer(StagingBuffer.Handle, StagingBuffer.Memory);
        
        // Create the Texture Sampler
        VkSamplerCreateInfo SamplerInfo = {};
        SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        SamplerInfo.magFilter               = VK_FILTER_LINEAR;
        SamplerInfo.minFilter               = VK_FILTER_LINEAR;
        SamplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        SamplerInfo.minLod                  = -1000;
        SamplerInfo.maxLod                  = 1000;
        SamplerInfo.maxAnisotropy           = 1.0f;
        DevGui->FontImage.Sampler = vk::CreateImageSampler(SamplerInfo);
        
        Io.Fonts->TexID = (ImTextureID)(intptr_t)DevGui->FontImage.Handle;
    }
    
    // Create the descriptors
    {
        VkDescriptorPoolSize DescriptorSizes[1];
        DescriptorSizes[0].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        DescriptorSizes[0].descriptorCount = 1; // Do I want more???
        DevGui->DescriptorPool = vk::CreateDescriptorPool(DescriptorSizes, 1, DevGui->SwapImageCount);
        
        VkSampler Sampler[1] = {DevGui->FontImage.Sampler};
        
        VkDescriptorSetLayoutBinding Bindings[1]= {};
        Bindings[0].binding            = 0;
        Bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        Bindings[0].descriptorCount    = 1;
        Bindings[0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
        Bindings[0].pImmutableSamplers = Sampler;
        DevGui->DescriptorLayout = vk::CreateDescriptorSetLayout(Bindings, 1);
        
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool     = DevGui->DescriptorPool;
        descriptorSetAllocateInfo.pSetLayouts        = &DevGui->DescriptorLayout;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        if (vk::CreateDescriptorSets(&DevGui->DescriptorSet, descriptorSetAllocateInfo) != VK_SUCCESS)
        {
            mprinte("Failed to create descriptor sets for the DevGui!\n");
        }
        
        VkDescriptorImageInfo ImageInfo = {};
        ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ImageInfo.imageView   = DevGui->FontImage.View;
        ImageInfo.sampler     = DevGui->FontImage.Sampler;
        
        VkWriteDescriptorSet DescriptorWrites = {};
        DescriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites.dstSet           = DevGui->DescriptorSet;
        DescriptorWrites.descriptorCount  = 1;
        DescriptorWrites.dstBinding       = 0;
        DescriptorWrites.dstArrayElement  = 0;
        DescriptorWrites.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        DescriptorWrites.pImageInfo       = &ImageInfo;
        vk::UpdateDescriptorSets(&DescriptorWrites, 1);
    }
    
    // Create the Pipeline
    {
        // Pipeline Cache
        vk::CreatePipelineCache(&DevGui->PipelineCache);
        
        // Pipeline Layout
        VkPushConstantRange PushConstantRange {};
        PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        PushConstantRange.offset     = 0;
        PushConstantRange.size       = sizeof(dev_ui_push_constant_block);
        
        VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
        PipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        PipelineLayoutInfo.setLayoutCount         = 1;
        PipelineLayoutInfo.pSetLayouts            = &DevGui->DescriptorLayout;
        PipelineLayoutInfo.pushConstantRangeCount = 1;
        PipelineLayoutInfo.pPushConstantRanges    = &PushConstantRange;
        DevGui->PipelineLayout = vk::CreatePipelineLayout(PipelineLayoutInfo);
        
        // Pipeline
        VkPipelineInputAssemblyStateCreateInfo InputAssembly = {};
        InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        InputAssembly.primitiveRestartEnable = VK_FALSE;
        
        VkPipelineRasterizationStateCreateInfo Rasterizer = {};
        Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        Rasterizer.depthClampEnable        = VK_FALSE;
        Rasterizer.rasterizerDiscardEnable = VK_FALSE;
        Rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
        Rasterizer.cullMode                = VK_CULL_MODE_NONE;
        Rasterizer.lineWidth               = 1.0f;
        Rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        Rasterizer.depthBiasEnable         = VK_FALSE;
        
        // Enable blending
        VkPipelineColorBlendAttachmentState BlendAttachmentState{};
        BlendAttachmentState.blendEnable = VK_TRUE;
        BlendAttachmentState.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
        BlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        BlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        BlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        BlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        BlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        BlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
        
        VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo {};
        PipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        PipelineColorBlendStateCreateInfo.attachmentCount = 1;
        PipelineColorBlendStateCreateInfo.pAttachments    = &BlendAttachmentState;
        
        VkPipelineDepthStencilStateCreateInfo DepthStencil = {};
        DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        
        VkPipelineViewportStateCreateInfo ViewportState = {};
        ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewportState.viewportCount = 1;
        ViewportState.scissorCount  = 1;
        
        VkPipelineMultisampleStateCreateInfo Multisampling = {};
        Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        Multisampling.sampleShadingEnable   = VK_FALSE;
        Multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
        
        VkDynamicState DynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        
        VkPipelineDynamicStateCreateInfo DynamicStateInfo;
        DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        DynamicStateInfo.pNext             = nullptr;
        DynamicStateInfo.flags             = 0;
        DynamicStateInfo.dynamicStateCount = 2;
        DynamicStateInfo.pDynamicStates    = DynamicStates;
        
        // Vertex bindings an attributes based on ImGui vertex definition
        VkVertexInputBindingDescription VertexInputBindings[1];
        VertexInputBindings[0] = {};
        VertexInputBindings[0].binding   = 0;
        VertexInputBindings[0].stride    = sizeof(ImDrawVert);
        VertexInputBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        VkVertexInputAttributeDescription VertexInputAttributes[3];
        
        VertexInputAttributes[0] = {};
        VertexInputAttributes[0].location = 0;
        VertexInputAttributes[0].binding  = 0;
        VertexInputAttributes[0].format   = VK_FORMAT_R32G32_SFLOAT;
        VertexInputAttributes[0].offset   = offsetof(ImDrawVert, pos);
        
        VertexInputAttributes[1] = {};
        VertexInputAttributes[1].location = 1;
        VertexInputAttributes[1].binding  = 0;
        VertexInputAttributes[1].format   = VK_FORMAT_R32G32_SFLOAT;
        VertexInputAttributes[1].offset   = offsetof(ImDrawVert, uv);
        
        VertexInputAttributes[2] = {};
        VertexInputAttributes[2].location = 2;
        VertexInputAttributes[2].binding  = 0;
        VertexInputAttributes[2].format   = VK_FORMAT_R8G8B8A8_UNORM;
        VertexInputAttributes[2].offset   = offsetof(ImDrawVert, col);
        
        VkPipelineVertexInputStateCreateInfo VertexInputState = {};
        VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VertexInputState.vertexBindingDescriptionCount   = 1;
        VertexInputState.pVertexBindingDescriptions      = VertexInputBindings;
        VertexInputState.vertexAttributeDescriptionCount = 3;
        VertexInputState.pVertexAttributeDescriptions    = VertexInputAttributes;
        
        // Create The Shader Modules:
#if 1
        jstring VertFile = InitJString("data/shaders/dev_ui.vert.spv");
        jstring FragFile = InitJString("data/shaders/dev_ui.frag.spv");
        
        jstring VertShaderFile = PlatformLoadFile(VertFile);
        jstring FragShaderFile = PlatformLoadFile(FragFile);
        
        VkShaderModule VertModule = vk::CreateShaderModule((u32*)VertShaderFile.GetCStr(),
                                                           VertShaderFile.len);
        VkShaderModule FragModule = vk::CreateShaderModule((u32*)FragShaderFile.GetCStr(),
                                                           FragShaderFile.len);
        
        VertFile.Clear();
        FragFile.Clear();
        VertShaderFile.Clear();
        FragShaderFile.Clear();
#endif
        
#if 0
        
        VkShaderModule VertModule = vk::CreateShaderModule((u32*)__glsl_shader_vert_spv,
                                                           sizeof(__glsl_shader_vert_spv));
        VkShaderModule FragModule = vk::CreateShaderModule((u32*)__glsl_shader_frag_spv,
                                                           sizeof(__glsl_shader_frag_spv));
        
#endif
        
        VkPipelineShaderStageCreateInfo ShaderStages[2];
        ShaderStages[0]        = {};
        ShaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderStages[0].module = VertModule;
        ShaderStages[0].pName  = "main";
        ShaderStages[1]        = {};
        ShaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        ShaderStages[1].module = FragModule;
        ShaderStages[1].pName  = "main";
        
        VkGraphicsPipelineCreateInfo PipelineCreateInfo = {};
        PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        PipelineCreateInfo.layout              = DevGui->PipelineLayout;
        PipelineCreateInfo.renderPass          = RenderPass;
        PipelineCreateInfo.flags               = 0;
        PipelineCreateInfo.basePipelineIndex   = -1;
        PipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
        PipelineCreateInfo.pInputAssemblyState = &InputAssembly;
        PipelineCreateInfo.pRasterizationState = &Rasterizer;
        PipelineCreateInfo.pColorBlendState    = &PipelineColorBlendStateCreateInfo;
        PipelineCreateInfo.pMultisampleState   = &Multisampling;
        PipelineCreateInfo.pViewportState      = &ViewportState;
        PipelineCreateInfo.pDepthStencilState  = &DepthStencil;
        PipelineCreateInfo.pDynamicState       = &DynamicStateInfo;
        PipelineCreateInfo.stageCount          = 2;
        PipelineCreateInfo.pStages             = ShaderStages;
        PipelineCreateInfo.pVertexInputState   = &VertexInputState;
        DevGui->Pipeline = vk::CreatePipeline(PipelineCreateInfo, DevGui->PipelineCache);
        
        vk::DestroyShaderModule(VertModule);
        vk::DestroyShaderModule(FragModule);
    }
}

void MapleDevGuiFree(maple_dev_gui *DevGui)
{
    vk::DestroyImageView(DevGui->FontImage.View);
    vk::DestroyImageSampler(DevGui->FontImage.Sampler);
    vk::DestroyVmaImage(DevGui->FontImage.Handle, DevGui->FontImage.Memory);
    
    for (u32 ImageIdx = 0; ImageIdx < DevGui->SwapImageCount; ++ImageIdx)
    {
        if (DevGui->VertexBuffer[ImageIdx].Handle)
            vk::DestroyVmaBuffer(DevGui->VertexBuffer[ImageIdx].Handle, DevGui->VertexBuffer[ImageIdx].Memory);
        
        if (DevGui->IndexBuffer[ImageIdx].Handle)
            vk::DestroyVmaBuffer(DevGui->IndexBuffer[ImageIdx].Handle, DevGui->IndexBuffer[ImageIdx].Memory);
    }
    
    vk::DestroyPipelineCache(DevGui->PipelineCache);
    vk::DestroyPipelineLayout(DevGui->PipelineLayout);
    vk::DestroyPipeline(DevGui->Pipeline);
    
    vk::DestroyDescriptorPool(DevGui->DescriptorPool);
    vk::DestroyDescriptorSetLayout(DevGui->DescriptorLayout);
    
    vk::DestroyCommandPool(DevGui->CommandPool);
    
    if (DevGui->VertexBuffer) pfree(DevGui->VertexBuffer);
    if (DevGui->IndexBuffer)  pfree(DevGui->IndexBuffer);
    DevGui->VertexBuffer = nullptr;
    DevGui->IndexBuffer  = nullptr;
    DevGui->VertexCount  = 0;
    DevGui->IndexCount   = 0;
}

file_internal void MapleDevGuiUpdateBuffers(BufferParameters &VertexBuffer, i32 &CurrentVertexCount,
                                            BufferParameters &IndexBuffer,  i32 &CurrentIndexCount)
{
    ImDrawData* imDrawData = ImGui::GetDrawData();
    
    // Note: Alignment is done inside buffer creation
    VkDeviceSize VertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
    VkDeviceSize IndexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
    
    if ((VertexBufferSize == 0) || (IndexBufferSize == 0)) return;
    
    // Vertex buffer
    if ((VertexBuffer.Handle == VK_NULL_HANDLE) || (CurrentVertexCount < imDrawData->TotalVtxCount))
    {
        if (VertexBuffer.Handle)
            vk::DestroyVmaBuffer(VertexBuffer.Handle, VertexBuffer.Memory);
        
        VkBufferCreateInfo vertex_buffer_info = {};
        vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertex_buffer_info.size  = VertexBufferSize;
        vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        
        VmaAllocationCreateInfo vertex_alloc_info = {};
        vertex_alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        vertex_alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        
        vk::CreateVmaBuffer(vertex_buffer_info,
                            vertex_alloc_info,
                            VertexBuffer.Handle,
                            VertexBuffer.Memory,
                            VertexBuffer.AllocationInfo);
        
        CurrentVertexCount = imDrawData->TotalVtxCount;
    }
    
    // Index buffer
    if ((IndexBuffer.Handle == VK_NULL_HANDLE) || (CurrentIndexCount < imDrawData->TotalIdxCount))
    {
        if (IndexBuffer.Handle)
            vk::DestroyVmaBuffer(IndexBuffer.Handle, IndexBuffer.Memory);
        
        VkBufferCreateInfo index_buffer_info = {};
        index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        index_buffer_info.size  = IndexBufferSize;
        index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        
        VmaAllocationCreateInfo index_alloc_info = {};
        index_alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        index_alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        
        vk::CreateVmaBuffer(index_buffer_info,
                            index_alloc_info,
                            IndexBuffer.Handle,
                            IndexBuffer.Memory,
                            IndexBuffer.AllocationInfo);
        
        CurrentIndexCount = imDrawData->TotalVtxCount;
    }
    
    // Upload the data
    ImDrawVert* vtxDst = (ImDrawVert*)VertexBuffer.AllocationInfo.pMappedData;
    ImDrawIdx* idxDst = (ImDrawIdx*)IndexBuffer.AllocationInfo.pMappedData;
    
    for (int n = 0; n < imDrawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = imDrawData->CmdLists[n];
        memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += cmd_list->VtxBuffer.Size;
        idxDst += cmd_list->IdxBuffer.Size;
    }
    
    // Flush the buffer
    vk::VmaFlushAllocation(VertexBuffer.Memory);
    vk::VmaFlushAllocation(IndexBuffer.Memory);
}

void MapleDevGuiDraw(maple_dev_gui *DevGui, VkCommandBuffer CommandBuffer)
{
    ImDrawData* ImDrawData = ImGui::GetDrawData();
    
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(ImDrawData->DisplaySize.x * ImDrawData->FramebufferScale.x);
    int fb_height = (int)(ImDrawData->DisplaySize.y * ImDrawData->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;
    
    ImGuiIO& io = ImGui::GetIO();
    
    // Update the Image Index for this buffer...
    DevGui->ImageIndex = (DevGui->ImageIndex + 1) % DevGui->SwapImageCount;
    BufferParameters &VertexBuffer = DevGui->VertexBuffer[DevGui->ImageIndex];
    BufferParameters &IndexBuffer  = DevGui->IndexBuffer[DevGui->ImageIndex];
    MapleDevGuiUpdateBuffers(VertexBuffer, DevGui->VertexCount,
                             IndexBuffer,  DevGui->IndexCount);
    
    vk::BindDescriptorSets(CommandBuffer, DevGui->PipelineLayout, 0, 1,
                           &DevGui->DescriptorSet, 0, nullptr);
    vk::BindPipeline(CommandBuffer, DevGui->Pipeline);
    
    {
        VkViewport Viewport {};
        Viewport.x = 0;
        Viewport.y = 0;
        Viewport.width    = (r32)fb_width;
        Viewport.height   = (r32)fb_height;
        Viewport.minDepth = 0.0f;
        Viewport.maxDepth = 1.0f;
        vk::SetViewport(CommandBuffer, 0, 1, &Viewport);
    }
    
    {
        // UI scale and translate via push constants
        dev_ui_push_constant_block PushConstants = {};
        PushConstants.Scale     = {
            2.0f / io.DisplaySize.x,
            2.0f / io.DisplaySize.y
        };
        
        PushConstants.Translate = {
            -1.0f - ImDrawData->DisplayPos.x * PushConstants.Scale[0],
            -1.0f - ImDrawData->DisplayPos.y * PushConstants.Scale[1]
        };
        
        vk::PushConstants(CommandBuffer, DevGui->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                          sizeof(dev_ui_push_constant_block), &PushConstants);
    }
    
    
    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 ClipOff   = ImDrawData->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 ClipScale = ImDrawData->FramebufferScale;   // (1,1) unless using retina display which are often (2,2)
    
    i32 VertexOffset = 0;
    i32 IndexOffset = 0;
    
    VkBuffer Buffers[1] = {VertexBuffer.Handle};
    VkDeviceSize Offsets[1] = { 0 };
    
    if (ImDrawData->TotalVtxCount > 0)
    {
        vk::BindVertexBuffers(CommandBuffer, 0, 1, Buffers, Offsets);
        vk::BindIndexBuffer(CommandBuffer, IndexBuffer.Handle, 0, VK_INDEX_TYPE_UINT16);
    }
    
    for (int32_t i = 0; i < ImDrawData->CmdListsCount; i++)
    {
        const ImDrawList* cmd_list = ImDrawData->CmdLists[i];
        for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
            
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - ClipOff.x) * ClipScale.x;
                clip_rect.y = (pcmd->ClipRect.y - ClipOff.y) * ClipScale.y;
                clip_rect.z = (pcmd->ClipRect.z - ClipOff.x) * ClipScale.x;
                clip_rect.w = (pcmd->ClipRect.w - ClipOff.y) * ClipScale.y;
                
                if (clip_rect.x < fb_width  &&
                    clip_rect.y < fb_height &&
                    clip_rect.z >= 0.0f     &&
                    clip_rect.w >= 0.0f)
                {
                    // Negative offsets are illegal for vkCmdSetScissor
                    if (clip_rect.x < 0.0f)
                        clip_rect.x = 0.0f;
                    if (clip_rect.y < 0.0f)
                        clip_rect.y = 0.0f;
                    
                    // Apply scissor/clipping rectangle
                    VkRect2D Scissor;
                    Scissor.offset.x      = (i32)(clip_rect.x);
                    Scissor.offset.y      = (i32)(clip_rect.y);
                    Scissor.extent.width  = (u32)(clip_rect.z - clip_rect.x);
                    Scissor.extent.height = (u32)(clip_rect.w - clip_rect.y);
                    vk::SetScissor(CommandBuffer, 0, 1, &Scissor);
                    
                    // Draw
                    vk::vkCmdDrawIndexed(CommandBuffer, pcmd->ElemCount, 1, pcmd->IdxOffset + IndexOffset, pcmd->VtxOffset + VertexOffset, 0);
                    
                    vk::DrawIndexed(CommandBuffer, pcmd->ElemCount, 1, pcmd->IdxOffset + IndexOffset,
                                    pcmd->VtxOffset + VertexOffset, 0);
                }
            }
        }
        
        IndexOffset  += cmd_list->IdxBuffer.Size;
        VertexOffset += cmd_list->VtxBuffer.Size;
    }
}
