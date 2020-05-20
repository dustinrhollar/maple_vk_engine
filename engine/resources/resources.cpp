
namespace mresource
{
    struct descriptor_sizes
    {
        VkDescriptorPoolSize DescriptorSize;
        u32                  AvailableDescriptors; // count of the available descriptors
    };
    
    struct descriptor_pool
    {
        VkDescriptorPool  Pool;
        descriptor_sizes *DescriptorSizes;
        u32               DescriptorSizesCount;
    };
    
    struct descriptor_pool_list
    {
        descriptor_pool *Pools;
        u32              Count;
        u32              Cap;
        
        // currently allocated types within the descriptor pool
        // This does not mean ALL pools contain these types, but
        // it is guarenteed that the most recently allocated desrciptor
        // pool does have all of these types.
        //VkDescriptorType AvailableTypes;
        u32              MaxSetType;
    } file_global PoolList;
    
    struct resource_registry
    {
        resource *Resources;
        u32       Count;
        u32       Cap;
    } file_global ResourceRegistry;
    
    file_internal void InitDescriptorPool(descriptor_pool *Pool);
    file_internal void FreeDescriptorPool(descriptor_pool *Pool);
    
    file_internal void InitRegistry(resource_registry *Registry, u32 Cap);
    file_internal void FreeRegistry(resource_registry *Registry);
    file_internal void RegistryResize(resource_registry *Registry, u32 NewSize);
    file_internal resource_id_t RegistryAdd(resource_registry *Registry, resource Resource);
    file_internal void RegistryRemove(resource_registry *Registry, u32 ResourceIdx);
    file_internal inline bool RegistryIsValidResource(resource_registry *Registry, resource_id_t Id);
    
    file_internal void InitDescriptorPoolList(descriptor_pool_list *PoolList, u32 Cap);
    file_internal void FreeDescriptorPoolList(descriptor_pool_list *PoolList);
    file_internal void DescriptorPoolListResize(descriptor_pool_list *PoolList, u32 NewSize);
    file_internal void DescriptorPoolListAdd(descriptor_pool_list *PoolList, descriptor_pool Pool);
    file_internal void DescriptorPoolListRemove(descriptor_pool_list *PoolList, u32 PoolIdx);
    
    file_internal void FreeResource(resource *Resource);
    
    file_internal void InitRegistry(resource_registry *Registry, u32 Cap)
    {
        Registry->Count     = 0;
        Registry->Cap       = Cap;
        Registry->Resources = palloc<resource>(Registry->Cap);
        
        for (u32 i = 0; i < Registry->Cap; ++i)
            Registry->Resources[i].Id = -1;
    }
    
    file_internal void FreeRegistry(resource_registry *Registry)
    {
        for (u32 i = 0; i < Registry->Cap; ++i)
            FreeResource(&Registry->Resources[i]);
        
        pfree(Registry->Resources);
        Registry->Count = 0;
        Registry->Cap   = 0;
    }
    
    file_internal void RegistryResize(resource_registry *Registry, u32 NewSize)
    {
        resource *NewRegistry = palloc<resource>(NewSize);
        
        for (u32 Idx = 0; Idx < Registry->Count; Idx++)
            NewRegistry[Idx] = Registry->Resources[Idx];
        
        pfree(Registry->Resources);
        Registry->Resources = NewRegistry;
        Registry->Cap       = NewSize;
    }
    
    file_internal resource_id_t RegistryAdd(resource_registry *Registry, resource Resource)
    {
        if (Registry->Count + 1 >= Registry->Cap)
            RegistryResize(Registry, Registry->Cap*2);
        
        resource_id_t Result = Registry->Count;
        Resource.Id = Result;
        Registry->Resources[Registry->Count++] = Resource;
        
        return Result;
    }
    
    file_internal void RegistryRemove(resource_registry *Registry, u32 ResourceIdx)
    {
        resource Resource = Registry->Resources[ResourceIdx];
        FreeResource(&Resource);
        Registry->Resources[ResourceIdx].Id = -1;
    }
    
    file_internal inline bool RegistryIsValidResource(resource_registry *Registry,
                                                      resource_id_t Id)
    {
        return Id < Registry->Count && Registry->Resources[Id].Id == Id;
    }
    
    
    file_internal void InitDescriptorPool(descriptor_pool *Pool,
                                          VkDescriptorPoolSize *DescriptorSizes, u32 SizeCount, u32 MaxSets)
    {
        // TODO(Dustin): Not done with this function yet
        Pool->Pool = vk::CreateDescriptorPool(DescriptorSizes, SizeCount, MaxSets,
                                              VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
    }
    
    file_internal void FreeDescriptorPool(descriptor_pool *Pool)
    {
        pfree(Pool->DescriptorSizes);
        vk::DestroyDescriptorPool(Pool->Pool);
        Pool->DescriptorSizesCount = 0;
        Pool->DescriptorSizes      = nullptr;
    }
    
    
    file_internal void DescriptorPoolListResize(descriptor_pool_list *PoolList, u32 NewSize)
    {
        descriptor_pool *NewPoolList = palloc<descriptor_pool>(NewSize);
        
        for (u32 PoolIdx = 0; PoolIdx < PoolList->Count; PoolIdx++)
            NewPoolList[PoolIdx] = PoolList->Pools[PoolIdx];
        
        pfree(PoolList->Pools);
        PoolList->Pools = NewPoolList;
        PoolList->Cap   = NewSize;
    }
    
    file_internal void DescriptorPoolListAdd(descriptor_pool_list *PoolList, descriptor_pool Pool)
    {
        PlatformPrintMessage(EConsoleColor::Yellow, EConsoleColor::DarkGrey, "A new descriptor pool is being added!\n");
        
        if (PoolList->Count + 1 >= PoolList->Cap)
            DescriptorPoolListResize(PoolList, PoolList->Cap*2);
        
        PoolList->Pools[PoolList->Count++] = Pool;
    }
    
    file_internal void DescriptorPoolListRemove(descriptor_pool_list *PoolList, u32 PoolIdx)
    {
        descriptor_pool PoolToRemove = PoolList->Pools[PoolIdx];
        if (PoolList->Count > 1)
            PoolList->Pools[PoolIdx] = PoolList->Pools[PoolList->Count-1];
        
        FreeDescriptorPool(&PoolToRemove);
        
        --PoolList->Count;
    }
    
    file_internal void InitDescriptorPoolList(descriptor_pool_list *PoolList, u32 Cap)
    {
        PoolList->Cap   = Cap;
        PoolList->Count = 0;
        PoolList->Pools = palloc<descriptor_pool>(PoolList->Cap);
    }
    
    file_internal void FreeDescriptorPoolList(descriptor_pool_list *PoolList)
    {
        for (u32 PoolIdx = 0; PoolIdx < PoolList->Count; ++PoolIdx)
            FreeDescriptorPool(&PoolList->Pools[PoolIdx]);
        
        pfree(PoolList->Pools);
        PoolList->Cap   = 0;
        PoolList->Count = 0;
        PoolList->Pools = nullptr;
    }
    
    void Init(frame_params FrameParams)
    {
        InitDescriptorPoolList(&PoolList, 3);
        
        //PoolList.AvailableTypes = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER |
        //VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC               |
        //VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        PoolList.MaxSetType = 10; // ten sets for each type is available
        
        VkDescriptorPoolSize DescriptorSizes[3];
        DescriptorSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        DescriptorSizes[0].descriptorCount = PoolList.MaxSetType;
        
        DescriptorSizes[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        DescriptorSizes[1].descriptorCount = PoolList.MaxSetType;
        
        DescriptorSizes[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        DescriptorSizes[2].descriptorCount = PoolList.MaxSetType;
        
        descriptor_pool Pool = {};
        InitDescriptorPool(&Pool,
                           DescriptorSizes, 3, 3 * PoolList.MaxSetType);
        
        DescriptorPoolListAdd(&PoolList, Pool);
    }
    
    
    resource_id_t Load(resource_type Type, void *Data)
    {
        resource_id_t Result = -1;
        resource Resource = {};
        Resource.Type = Type;
        
        switch (Type)
        {
            case Resource_DescriptorSetLayout:
            {
                descriptor_layout_create_info *Info =
                    static_cast<descriptor_layout_create_info*>(Data);
                
                // NOTE(Dustin): Do I want to move this to a GpuCommand?
                VkDescriptorSetLayout Layout =
                    vk::CreateDescriptorSetLayout(Info->Bindings, Info->BindingsCount);
                
                Resource.DescriptorLayout = { Layout };
                
                Result = RegistryAdd(&ResourceRegistry, Resource);
            } break;
            
            case Resource_DescriptorSet:
            {
                descriptor_create_info *Info =
                    static_cast<descriptor_create_info*>(Data);
                
                resource_descriptor_set RDescriptorSet = {};
                RDescriptorSet.DescriptorSetsCount = Info->SetCount;
                RDescriptorSet.DescriptorSets = palloc<DescriptorSetParameters>(Info->SetCount);
                
                for (u32 Set = 0; Set < Info->SetCount; ++Set)
                {
                    VkResult SetAllocResult;
                    VkDescriptorPool DescriptorPool;
                    
                    resource ResourceLayout =
                        ResourceRegistry.Resources[Info->DescriptorLayouts[Set]];
                    VkDescriptorSetLayout Layout = ResourceLayout.DescriptorLayout.DescriptorLayout;
                    
                    for (u32 Pool = 0; Pool < PoolList.Count; ++Pool)
                    {
                        DescriptorPool = PoolList.Pools[Pool].Pool;
                        
                        VkDescriptorSetAllocateInfo allocInfo = {};
                        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                        allocInfo.descriptorPool     = DescriptorPool;
                        allocInfo.descriptorSetCount = 1;
                        allocInfo.pSetLayouts        = &Layout;
                        
                        // NOTE(Dustin): Do I want to move this to a GpuCommand?
                        SetAllocResult =
                            vk::CreateDescriptorSets(&RDescriptorSet.DescriptorSets[Set].Handle,
                                                     allocInfo);
                        
                        if (SetAllocResult == VK_SUCCESS)
                            break;
                        
                        PlatformPrintMessage(EConsoleColor::Yellow, EConsoleColor::DarkGrey,
                                             "Failed to allocate a descriptor set, trying next pool!\n");
                    }
                    
                    if (SetAllocResult != VK_SUCCESS)
                    {
                        PlatformPrintMessage(EConsoleColor::Yellow, EConsoleColor::DarkGrey,
                                             "Failed to allocate a descriptor set from any of the pools! Creating a new pool...\n");
                        // TODO(Dustin): Create New Descriptor Pool
                        
                        
                        // TODO(Dustin): Attempt to allocate the descriptorset from the new pool...
                    }
                    
                    RDescriptorSet.DescriptorSets[Set].Layout = Layout;
                    RDescriptorSet.DescriptorSets[Set].Pool   = DescriptorPool;
                }
                
                Resource.DescriptorSet = RDescriptorSet;
                Result = RegistryAdd(&ResourceRegistry, Resource);
            } break;
            
            case Resource_VertexBuffer:
            {
            } break;
            
            case Resource_IndexBuffer:
            {
            } break;
            
            case Resource_UniformBuffer:
            {
                buffer_create_info *Info =
                    static_cast<buffer_create_info*>(Data);
                
                resource_buffer RBuffer = {};
                RBuffer.PersistentlyMapped = Info->PersistentlyMapped;
                RBuffer.BufferCount        = Info->BufferCount;
                RBuffer.Buffers            = palloc<BufferParameters>(Info->BufferCount);
                
                for (u32 Buffer = 0; Buffer < Info->BufferCount; ++Buffer)
                {
                    
                    VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
                    create_info.size  = Info->SizePerBuffer;
                    create_info.usage = Info->Usage;
                    
                    VmaAllocationCreateInfo alloc_info = {};
                    alloc_info.usage = Info->Properties;
                    
                    if (RBuffer.PersistentlyMapped)
                        alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
                    
                    vk::CreateVmaBuffer(create_info,
                                        alloc_info,
                                        RBuffer.Buffers[Buffer].Handle,
                                        RBuffer.Buffers[Buffer].Memory,
                                        RBuffer.Buffers[Buffer].AllocationInfo);
                }
                
                Resource.UniformBuffer = RBuffer;
                Result = RegistryAdd(&ResourceRegistry, Resource);
            } break;
            
            case Resource_DynamicUniformBuffer:
            {
            } break;
            
            case Resource_Image:
            {
            } break;
            
            case Resource_Pipeline:
            {
                pipeline_create_info *Info =
                    static_cast<pipeline_create_info*>(Data);
                
                VkShaderModule                  *ShaderModules =
                    talloc<VkShaderModule>(Info->ShadersCount);
                VkPipelineShaderStageCreateInfo *ShaderStages  =
                    talloc<VkPipelineShaderStageCreateInfo>(Info->ShadersCount);
                
                for (u32 Shader = 0; Shader < Info->ShadersCount; Shader++)
                {
                    jstring ShaderFile = PlatformLoadFile(Info->Shaders[Shader].Filename);
                    
                    ShaderModules[Shader] =
                        vk::CreateShaderModule(ShaderFile.GetCStr(), ShaderFile.len);
                    
                    VkPipelineShaderStageCreateInfo ShaderStageInfo = {};
                    ShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                    ShaderStageInfo.stage  = Info->Shaders[Shader].ShaderStage;
                    ShaderStageInfo.module = ShaderModules[Shader];
                    ShaderStageInfo.pName  = "main";
                    
                    ShaderStages[Shader] = ShaderStageInfo;
                    
                    ShaderFile.Clear();
                }
                
                VkPipelineInputAssemblyStateCreateInfo InputAssembly = {};
                InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                InputAssembly.primitiveRestartEnable = VK_FALSE;
                
                VkPipelineViewportStateCreateInfo ViewportState = {};
                ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                ViewportState.viewportCount = Info->ViewportCount;
                ViewportState.pViewports    = Info->Viewport;
                ViewportState.scissorCount  = Info->ScissorCount;
                ViewportState.pScissors     = Info->Scissor;
                
                // Rasterizer
                VkPipelineRasterizationStateCreateInfo Rasterizer = {};
                Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                Rasterizer.depthClampEnable        = VK_FALSE;
                Rasterizer.rasterizerDiscardEnable = VK_FALSE;
                Rasterizer.polygonMode             = Info->PolygonMode;
                Rasterizer.lineWidth               = 1.0f;
                Rasterizer.frontFace               = Info->FrontFace;
                Rasterizer.depthBiasEnable         = VK_FALSE;
                Rasterizer.depthBiasConstantFactor = 0.0f; // Optional
                Rasterizer.depthBiasClamp          = 0.0f; // Optional
                Rasterizer.depthBiasSlopeFactor    = 0.0f; // Optional
                
                VkPipelineMultisampleStateCreateInfo Multisampling = {};
                Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                Multisampling.sampleShadingEnable   = VK_FALSE;
                Multisampling.minSampleShading      = 1.0f; // Optional
                Multisampling.pSampleMask           = nullptr; // Optional
                Multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
                Multisampling.alphaToOneEnable      = VK_FALSE; // Optional
                
                if (Info->HasMultisampling)
                {
                    Multisampling.rasterizationSamples = Info->MuliSampleSamples;
                }
                else
                {
                    Multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
                }
                
                // Depth/Stencil Testing - not right now
                // TODO(Dustin): What to do when depth stencil is disabled....?
                VkPipelineDepthStencilStateCreateInfo DepthStencil = {};
                DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                DepthStencil.depthTestEnable = VK_TRUE;
                DepthStencil.depthWriteEnable = VK_TRUE;
                DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
                DepthStencil.depthBoundsTestEnable = VK_FALSE;
                DepthStencil.stencilTestEnable = VK_FALSE;
                
                // Color Blending
                VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
                ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                    VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                ColorBlendAttachment.blendEnable = VK_FALSE;
                
                VkPipelineColorBlendStateCreateInfo ColorBlending = {};
                ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                ColorBlending.logicOpEnable = VK_FALSE;
                ColorBlending.logicOp = VK_LOGIC_OP_COPY;
                ColorBlending.attachmentCount = 1;
                ColorBlending.pAttachments = &ColorBlendAttachment;
                ColorBlending.blendConstants[0] = 0.0f;
                ColorBlending.blendConstants[1] = 0.0f;
                ColorBlending.blendConstants[2] = 0.0f;
                ColorBlending.blendConstants[3] = 0.0f;
                
                VkDynamicState dynamic_states[] = {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR
                };
                
                VkPipelineDynamicStateCreateInfo DynamicStateInfo;
                DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                DynamicStateInfo.pNext = nullptr;
                DynamicStateInfo.flags = 0;
                DynamicStateInfo.dynamicStateCount = 2;
                DynamicStateInfo.pDynamicStates = dynamic_states;
                
                
                VkDescriptorSetLayout *Layouts =
                    talloc<VkDescriptorSetLayout>(Info->DescriptorLayoutsCount);
                for (u32 Set = 0; Set < Info->DescriptorLayoutsCount; ++Set)
                {
                    resource ResourceLayout =
                        ResourceRegistry.Resources[Info->DescriptorLayoutIds[Set]];
                    
                    Layouts[Set] = ResourceLayout.DescriptorLayout.DescriptorLayout;
                }
                
                VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
                PipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                PipelineLayoutInfo.setLayoutCount         = Info->DescriptorLayoutsCount;
                PipelineLayoutInfo.pSetLayouts            = Layouts;
                PipelineLayoutInfo.pushConstantRangeCount = Info->PushConstantsCount;
                PipelineLayoutInfo.pPushConstantRanges    = Info->PushConstants;
                
                VkPipelineLayout vPipelineLayout = vk::CreatePipelineLayout(PipelineLayoutInfo);
                
                VkGraphicsPipelineCreateInfo PipelineInfo = {};
                PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                PipelineInfo.stageCount          = Info->ShadersCount;
                PipelineInfo.pStages             = ShaderStages;
                PipelineInfo.pVertexInputState   = &Info->VertexInputInfo;
                PipelineInfo.pInputAssemblyState = &InputAssembly;
                PipelineInfo.pViewportState      = &ViewportState;
                PipelineInfo.pRasterizationState = &Rasterizer;
                PipelineInfo.pMultisampleState   = &Multisampling;
                PipelineInfo.pDepthStencilState  = &DepthStencil;
                PipelineInfo.pColorBlendState    = &ColorBlending;
                PipelineInfo.pDynamicState       = &DynamicStateInfo; // Optional
                PipelineInfo.layout              = vPipelineLayout;
                PipelineInfo.renderPass          = Info->RenderPass;
                PipelineInfo.subpass             = 0;
                PipelineInfo.basePipelineHandle  = VK_NULL_HANDLE; // Optional
                PipelineInfo.basePipelineIndex   = -1; // Optional
                
                VkPipeline vPipeline = vk::CreatePipeline(PipelineInfo);
                
                resource_pipeline rPipeline = {};
                rPipeline.Layout   = vPipelineLayout;
                rPipeline.Pipeline = vPipeline;
                
                Resource.Pipeline = rPipeline;
                Result = RegistryAdd(&ResourceRegistry, Resource);
                
                // Destroy the shader stage modules
                for (u32 Shader = 0; Shader < Info->ShadersCount; Shader++)
                {
                    vk::DestroyShaderModule(ShaderModules[Shader]);
                }
            } break;
            
            default: break;
        }
        
        return Result;
    }
    
    
    file_internal void FreeResource(resource *Resource)
    {
        
    }
    
}; // mresource