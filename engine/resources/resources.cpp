
namespace mm
{
    void InitDynUniformPool(dyn_uniform_pool *Pool, dynamic_buffer_create_info *Info)
    {
        u64 MinAlignment  = vk::GetMinUniformMemoryOffsetAlignment();
        
        Pool->Alignment   = (MinAlignment + Info->ElementStride - 1) & ~(MinAlignment - 1);
        Pool->ElementSize = Info->ElementStride;
        Pool->Offset      = 0;
        
        u64 BufferSize = Pool->Alignment * Info->ElementCount;
        
        VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        create_info.size  = BufferSize;
        create_info.usage = Info->Usage;
        
        VmaAllocationCreateInfo alloc_info = {};
        alloc_info.usage = Info->MemoryUsage;
        alloc_info.flags = Info->MemoryFlags;
        
        if (Info->PersistentlyMapped)
            alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        
        BufferParameters Buffer = {};
        vk::CreateVmaBuffer(create_info,
                            alloc_info,
                            Buffer.Handle,
                            Buffer.Memory,
                            Buffer.AllocationInfo);
        Buffer.Size = BufferSize;
        
        Pool->Buffer = Buffer;
    }
    
    void FreeDynUniformPool(dyn_uniform_pool *Pool)
    {
        vk::DestroyVmaBuffer(Pool->Buffer.Handle,
                             Pool->Buffer.Memory);
    }
    
    void AllocDynUniformPool(dyn_uniform_pool *Pool, void *Data, u32 Offset, bool IsMapped)
    {
        if (IsMapped)
        {
            memcpy((char*)Pool->Buffer.AllocationInfo.pMappedData + Offset, Data,
                   Pool->ElementSize);
        }
        else
        {
            void *BufferPtr = nullptr;
            vk::VmaMap(&BufferPtr, Pool->Buffer.Memory);
            {
                memcpy((char*)BufferPtr + Offset, Data, Pool->ElementSize);
            }
            vk::VmaUnmap(Pool->Buffer.Memory);
        }
    }
    
    void ResetDynUniformPool(dyn_uniform_pool *Pool, u32 Offset)
    {
        Pool->Offset = Offset;
    }
    
}; // mm

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
        resource **Resources;
        u32        Count;
        u32        Cap;
        
        // Default Global Descriptor, contains VP buffer
        resource_id_t DefaultGlobalDescriptor;
        resource_id_t DefaultGlobalDescriptorLayout;
        
        // Set = 1 is reserved for Model Descriptor
        // All renderable objects that is not a compute
        // material will use this descriptor.
        resource_id_t ObjectDescriptor;
        resource_id_t ObjectDescriptorLayout;
        
        resource_id_t DefaultGlobalBuffer;
        resource_id_t ObjectDynamicBuffer;
        
        resource_id_t DefaultPipeline; // (Different from a default material)
    } file_global ResourceRegistry;
    
    file_global mm::resource_allocator ResourceAllocator;
    
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
        Registry->Resources = palloc<resource*>(Registry->Cap);
        
        mm::InitResourceAllocator(&ResourceAllocator, sizeof(resource), Cap);
        
        for (u32 i = 0; i < Registry->Cap; ++i)
            Registry->Resources[i] = nullptr;
    }
    
    file_internal void FreeRegistry(resource_registry *Registry)
    {
        for (u32 i = 0; i < Registry->Cap; ++i)
        {
            if (Registry->Resources[i])
            {
                FreeResource(Registry->Resources[i]);
                Registry->Resources[i] = nullptr;
            }
        }
        
        pfree(Registry->Resources);
        Registry->Count = 0;
        Registry->Cap   = 0;
    }
    
    file_internal void RegistryResize(resource_registry *Registry, u32 NewSize)
    {
        resource **NewRegistry = palloc<resource*>(NewSize);
        
        for (u32 Idx = 0; Idx < Registry->Count; Idx++)
            NewRegistry[Idx] = Registry->Resources[Idx];
        
        // Mark empty slots as invalid resources
        for (u32 Idx = Registry->Count; Idx < NewSize; ++Idx)
            NewRegistry[Idx] = nullptr;
        
        pfree(Registry->Resources);
        Registry->Resources = NewRegistry;
        Registry->Cap       = NewSize;
    }
    
    file_internal resource_id_t RegistryAdd(resource_registry *Registry, resource Resource)
    {
        if (Registry->Count + 1 >= Registry->Cap)
            RegistryResize(Registry, (Registry->Cap>0) ? Registry->Cap*2 : 20);
        
        resource *NewResource = (resource*)ResourceAllocatorAlloc(&ResourceAllocator);
        *NewResource = Resource;
        
        resource_id_t Result = Registry->Count;
        NewResource->Id = Result;
        Registry->Resources[Registry->Count++] = NewResource;
        
        return Result;
    }
    
    file_internal void RegistryRemove(resource_registry *Registry, u32 ResourceIdx)
    {
        resource *Resource = Registry->Resources[ResourceIdx];
        FreeResource(Resource);
        ResourceAllocatorFree(&ResourceAllocator, Resource);
        Registry->Resources[ResourceIdx]->Id = -1;
    }
    
    file_internal inline bool RegistryIsValidResource(resource_registry *Registry,
                                                      resource_id_t Id)
    {
        return Id < Registry->Count && Registry->Resources[Id]->Id == Id;
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
            DescriptorPoolListResize(PoolList, (PoolList->Cap>0) ? PoolList->Cap*2 : 10);
        
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
    
    void Init(frame_params *FrameParams)
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
        
        InitRegistry(&ResourceRegistry, 10);
        
        //~ Global Data Uniform Buffer
        
        // TODO(Dustin): Size is hardcoded right now...
        buffer_create_info MvpBufferCreateInfo = {};
        MvpBufferCreateInfo.BufferSize         = sizeof(mat4) * 2;
        MvpBufferCreateInfo.PersistentlyMapped = true;
        MvpBufferCreateInfo.Usage              = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        MvpBufferCreateInfo.MemoryUsage        = VMA_MEMORY_USAGE_CPU_TO_GPU;
        MvpBufferCreateInfo.MemoryFlags        = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        
        ResourceRegistry.DefaultGlobalBuffer = mresource::Load({},
                                                               Resource_UniformBuffer,
                                                               &MvpBufferCreateInfo);
        
        //~ Object Uniform Buffer
        
        // TODO(Dustin): Size is hard coded...
        dynamic_buffer_create_info ModelBufferCreateInfo = {};
        ModelBufferCreateInfo.ElementCount       = 50;
        ModelBufferCreateInfo.ElementStride      = sizeof(mat4);
        ModelBufferCreateInfo.PersistentlyMapped = true;
        ModelBufferCreateInfo.Usage              = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        ModelBufferCreateInfo.MemoryUsage        = VMA_MEMORY_USAGE_CPU_TO_GPU;
        ModelBufferCreateInfo.MemoryFlags        = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        
        ResourceRegistry.ObjectDynamicBuffer = mresource::Load(FrameParams,
                                                               Resource_DynamicUniformBuffer,
                                                               &ModelBufferCreateInfo);
        
        //~ Default Global Descriptor, contains VP buffer
        
        VkDescriptorSetLayoutBinding Bindings[1]= {};
        Bindings[0].binding            = 0;
        Bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        Bindings[0].descriptorCount    = 1;
        Bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
        Bindings[0].pImmutableSamplers = nullptr; // Optional
        
        descriptor_layout_create_info DescriptorLayoutCreateInfo = {};
        DescriptorLayoutCreateInfo.BindingsCount = 1;
        DescriptorLayoutCreateInfo.Bindings      = Bindings;
        
        ResourceRegistry.DefaultGlobalDescriptorLayout = mresource::Load(FrameParams,
                                                                         Resource_DescriptorSetLayout,
                                                                         &DescriptorLayoutCreateInfo);
        
        descriptor_create_info DescriptorCreateInfo = {};
        DescriptorCreateInfo.DescriptorLayouts = &ResourceRegistry.DefaultGlobalDescriptorLayout;
        DescriptorCreateInfo.SetCount          = 1;
        
        ResourceRegistry.DefaultGlobalDescriptor = mresource::Load(FrameParams,
                                                                   Resource_DescriptorSet,
                                                                   &DescriptorCreateInfo);
        
        //~ Object Descriptor Info
        
        Bindings[0].binding            = 0;
        Bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        Bindings[0].descriptorCount    = 1;
        Bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
        Bindings[0].pImmutableSamplers = nullptr; // Optional
        
        DescriptorLayoutCreateInfo.BindingsCount = 1;
        DescriptorLayoutCreateInfo.Bindings      = Bindings;
        
        ResourceRegistry.ObjectDescriptorLayout = mresource::Load(FrameParams,
                                                                  Resource_DescriptorSetLayout,
                                                                  &DescriptorLayoutCreateInfo);
        
        DescriptorCreateInfo.DescriptorLayouts = &ResourceRegistry.ObjectDescriptorLayout;
        DescriptorCreateInfo.SetCount          = 1;
        
        ResourceRegistry.ObjectDescriptor = mresource::Load(FrameParams, Resource_DescriptorSet,
                                                            &DescriptorCreateInfo);
        
        //~ Update the descriptor write info for both sets of descriptors
        
        descriptor_write_info WriteInfos[2] = {
            {
                ResourceRegistry.DefaultGlobalBuffer,
                ResourceRegistry.DefaultGlobalDescriptor,
                0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
            },
            {
                ResourceRegistry.ObjectDynamicBuffer,
                ResourceRegistry.ObjectDescriptor,
                0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC }
        };
        
        descriptor_update_write_info DescriptorUpdateInfos = {};
        DescriptorUpdateInfos.WriteInfos      = WriteInfos;
        DescriptorUpdateInfos.WriteInfosCount = 2;
        
        mresource::Load(FrameParams, Resource_DescriptorSetWriteUpdate, &DescriptorUpdateInfos);
        
        //~ HACK(Dustin): Temporary material creation in order to allow for temp.
        // default material
        
        jstring VERT_SHADER = InitJString("data/shaders/shader.vert.spv");
        jstring FRAG_SHADER = InitJString("data/shaders/shader.frag.spv");
        
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
            ResourceRegistry.DefaultGlobalDescriptorLayout,
            ResourceRegistry.ObjectDescriptorLayout,
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
        
        ResourceRegistry.DefaultPipeline = mresource::Load({},
                                                           Resource_Pipeline,
                                                           &PipelineCreateInfo);
        
        VERT_SHADER.Clear();
        FRAG_SHADER.Clear();
    }
    
    void Free(frame_params *FrameParams)
    {
        vk::Idle();
        
        FreeRegistry(&ResourceRegistry);
        mm::FreeResourceAllocator(&ResourceAllocator);
        FreeDescriptorPoolList(&PoolList);
    }
    
    
    resource_id_t Load(frame_params *FrameParams, resource_type Type, void *Data)
    {
        u32 SwapchainImageCount = vk::GetSwapChainImageCount();
        
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
                RDescriptorSet.DescriptorSetsCount = SwapchainImageCount;
                RDescriptorSet.DescriptorSets = palloc<DescriptorSetParameters>(SwapchainImageCount);
                
                VkResult SetAllocResult;
                VkDescriptorPool DescriptorPool;
                
                resource *ResourceLayout =
                    ResourceRegistry.Resources[Info->DescriptorLayouts[0]];
                VkDescriptorSetLayout Layout = ResourceLayout->DescriptorLayout.DescriptorLayout;
                
                VkDescriptorSet *Sets = talloc<VkDescriptorSet>(SwapchainImageCount);
                VkDescriptorSetLayout *Layouts = talloc<VkDescriptorSetLayout>(SwapchainImageCount);
                for (u32 LayoutIdx = 0; LayoutIdx < SwapchainImageCount; ++LayoutIdx)
                    Layouts[LayoutIdx] = Layout;
                
                for (u32 Pool = 0; Pool < PoolList.Count; ++Pool)
                {
                    DescriptorPool = PoolList.Pools[Pool].Pool;
                    
                    VkDescriptorSetAllocateInfo allocInfo = {};
                    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    allocInfo.descriptorPool     = DescriptorPool;
                    allocInfo.descriptorSetCount = SwapchainImageCount;
                    allocInfo.pSetLayouts        = Layouts;
                    
                    // NOTE(Dustin): Do I want to move this to a GpuCommand?
                    SetAllocResult = vk::CreateDescriptorSets(Sets, allocInfo);
                    
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
                
                for (u32 Image = 0; Image < SwapchainImageCount; ++Image)
                {
                    RDescriptorSet.DescriptorSets[Image].Handle = Sets[Image];;
                    RDescriptorSet.DescriptorSets[Image].Layout = Layout;
                    RDescriptorSet.DescriptorSets[Image].Pool   = DescriptorPool;
                }
                
                Resource.DescriptorSet = RDescriptorSet;
                Result = RegistryAdd(&ResourceRegistry, Resource);
            } break;
            
            case Resource_DescriptorSetWriteUpdate:
            {
                descriptor_update_write_info *Infos =
                    static_cast<descriptor_update_write_info*>(Data);
                
                for (u32 SwapImage = 0; SwapImage < SwapchainImageCount; ++SwapImage)
                {
                    VkWriteDescriptorSet *DescriptorWrites =
                        talloc<VkWriteDescriptorSet>(Infos->WriteInfosCount);
                    
                    for (u32 WriteInfo = 0; WriteInfo < Infos->WriteInfosCount; WriteInfo++)
                    {
                        resource *UniformResource =
                            ResourceRegistry.Resources[Infos->WriteInfos[WriteInfo].BufferId];
                        
                        resource *DescriptorResource =
                            ResourceRegistry.Resources[Infos->WriteInfos[WriteInfo].DescriptorId];
                        
                        // Set 0: View-Projection Matrices
                        VkDescriptorBufferInfo *BufferInfo = talloc<VkDescriptorBufferInfo>(1);
                        *BufferInfo = {};
                        if (UniformResource->Type == Resource_UniformBuffer)
                        {
                            BufferInfo->buffer =
                                UniformResource->UniformBuffer.Buffers[SwapImage].Handle;
                        }
                        else if (UniformResource->Type == Resource_DynamicUniformBuffer)
                        {
                            BufferInfo->buffer =
                                UniformResource->DynamicUniformBuffer.Pools[SwapImage].Buffer.Handle;
                        }
                        else
                            mprinte("Attempting to update an incorrect buffer type with a Descriptor!\n");
                        
                        BufferInfo->offset = 0;
                        BufferInfo->range  = VK_WHOLE_SIZE;
                        
                        DescriptorWrites[WriteInfo] = {};
                        DescriptorWrites[WriteInfo].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        DescriptorWrites[WriteInfo].dstSet           =
                            DescriptorResource->DescriptorSet.DescriptorSets[SwapImage].Handle;
                        DescriptorWrites[WriteInfo].dstBinding       =
                            Infos->WriteInfos[WriteInfo].DescriptorBinding;
                        DescriptorWrites[WriteInfo].dstArrayElement  = 0;
                        DescriptorWrites[WriteInfo].descriptorType   =
                            Infos->WriteInfos[WriteInfo].DescriptorType;
                        DescriptorWrites[WriteInfo].descriptorCount  = 1;
                        DescriptorWrites[WriteInfo].pBufferInfo      = BufferInfo;
                        DescriptorWrites[WriteInfo].pImageInfo       = nullptr; // Optional
                        DescriptorWrites[WriteInfo].pTexelBufferView = nullptr; // Optional
                    }
                    
                    vk::UpdateDescriptorSets(DescriptorWrites, Infos->WriteInfosCount);
                }
            } break;
            
            case Resource_VertexBuffer:
            {
                vertex_buffer_create_info *Info =
                    static_cast<vertex_buffer_create_info*>(Data);
                
                VkBufferCreateInfo vertex_buffer_info = {};
                vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                vertex_buffer_info.size  = Info->Size;
                vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                
                VmaAllocationCreateInfo vertex_alloc_info = {};
                vertex_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
                
                // TODO(Dustin): Might want to have some flag that says if the resource has been uploaded
                // to the gpu yet.
                Resource.VertexBuffer = {}; // doesn't quite get initialized yet
                Result = RegistryAdd(&ResourceRegistry, Resource);
                
                gpu_vertex_buffer_create_info *VertexBufferCreateInfo = talloc<gpu_vertex_buffer_create_info>(1);
                VertexBufferCreateInfo->BufferCreateInfo = vertex_buffer_info;
                VertexBufferCreateInfo->VmaCreateInfo    = vertex_alloc_info;
                
                VertexBufferCreateInfo->BufferParams = &ResourceRegistry.Resources[Result]->VertexBuffer.Buffer;
                
                //VertexBufferCreateInfo->Buffer           = &ResourceRegistry.Resources[Result].VertexBuffer.Buffer.Handle;
                //VertexBufferCreateInfo->Allocation       = &ResourceRegistry.Resources[Result].VertexBuffer.Buffer.Memory;
                
                VertexBufferCreateInfo->Data             = Info->Data;
                VertexBufferCreateInfo->Size             = Info->Size;
                
                AddGpuCommand(FrameParams, { GpuCmd_UploadVertexBuffer, VertexBufferCreateInfo });
            } break;
            
            case Resource_IndexBuffer:
            {
                index_buffer_create_info *Info =
                    static_cast<index_buffer_create_info*>(Data);
                
                VkBufferCreateInfo index_buffer_info = {};
                index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                index_buffer_info.size  = Info->Size;
                index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                
                VmaAllocationCreateInfo index_alloc_info = {};
                index_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
                
                // TODO(Dustin): Might want to have some flag that says if the resource has been uploaded
                // to the gpu yet.
                Resource.IndexBuffer = {}; // doesn't quite get initialized yet
                Result = RegistryAdd(&ResourceRegistry, Resource);
                
                gpu_index_buffer_create_info *IndexBufferCreateInfo = talloc<gpu_index_buffer_create_info>(1);
                IndexBufferCreateInfo->BufferCreateInfo = index_buffer_info;
                IndexBufferCreateInfo->VmaCreateInfo    = index_alloc_info;
                IndexBufferCreateInfo->Buffer           = &ResourceRegistry.Resources[Result]->IndexBuffer.Buffer.Handle;
                IndexBufferCreateInfo->Allocation       = &ResourceRegistry.Resources[Result]->IndexBuffer.Buffer.Memory;
                IndexBufferCreateInfo->Data             = Info->Data;
                IndexBufferCreateInfo->Size             = Info->Size;
                
                AddGpuCommand(FrameParams, { GpuCmd_UploadIndexBuffer, IndexBufferCreateInfo });
            } break;
            
            case Resource_UniformBuffer:
            {
                buffer_create_info *Info =
                    static_cast<buffer_create_info*>(Data);
                
                resource_buffer RBuffer = {};
                RBuffer.PersistentlyMapped = Info->PersistentlyMapped;
                RBuffer.BufferCount        = SwapchainImageCount;
                RBuffer.Buffers            = palloc<BufferParameters>(SwapchainImageCount);
                
                for (u32 Buffer = 0; Buffer < SwapchainImageCount; ++Buffer)
                {
                    VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
                    create_info.size  = Info->BufferSize;
                    create_info.usage = Info->Usage;
                    
                    VmaAllocationCreateInfo alloc_info = {};
                    alloc_info.usage = Info->MemoryUsage;
                    alloc_info.flags = Info->MemoryFlags;
                    
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
                dynamic_buffer_create_info *Info =
                    static_cast<dynamic_buffer_create_info*>(Data);
                
                resource_dynamic_buffer RBuffer = {};
                RBuffer.PersistentlyMapped = Info->PersistentlyMapped;
                RBuffer.PoolsCount         = SwapchainImageCount;
                RBuffer.Pools              = palloc<mm::dyn_uniform_pool>(SwapchainImageCount);
                
                for (u32 Pool = 0; Pool < SwapchainImageCount; ++Pool)
                {
                    if (!RBuffer.PersistentlyMapped)
                        mprinte("Attempting to create a dynamic uniform buffer that is not persistently mapped. This is not currently supported! Please map the buffer.\n");
                    
                    mm::InitDynUniformPool(&RBuffer.Pools[Pool], Info);
                }
                
                Resource.DynamicUniformBuffer = RBuffer;
                Result = RegistryAdd(&ResourceRegistry, Resource);
            } break;
            
            case Resource_Image:
            {
            } break;
            
            case Resource_Pipeline:
            {
                pipeline_create_info *Info = static_cast<pipeline_create_info*>(Data);
                
                VkShaderModule *ShaderModules = talloc<VkShaderModule>(Info->ShadersCount);
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
                DynamicStateInfo.pNext             = nullptr;
                DynamicStateInfo.flags             = 0;
                DynamicStateInfo.dynamicStateCount = 2;
                DynamicStateInfo.pDynamicStates    = dynamic_states;
                
                VkDescriptorSetLayout *Layouts =
                    talloc<VkDescriptorSetLayout>(Info->DescriptorLayoutsCount);
                for (u32 Set = 0; Set < Info->DescriptorLayoutsCount; ++Set)
                {
                    resource *ResourceLayout =
                        ResourceRegistry.Resources[Info->DescriptorLayoutIds[Set]];
                    
                    Layouts[Set] = ResourceLayout->DescriptorLayout.DescriptorLayout;
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
            
            default:
            {
                mprinte("Unknown resource type %d\n", Type);
            } break;
        }
        
        return Result;
    }
    
    file_internal void FreeResource(resource *Resource)
    {
        // NOTE(Dustin): Probably need tyo unify this...
        vk::Idle();
        
        switch (Resource->Type)
        {
            case Resource_DescriptorSetLayout:
            {
                vk::DestroyDescriptorSetLayout(Resource->DescriptorLayout.DescriptorLayout);
            } break;
            
            case Resource_DescriptorSet:
            {
                for (u32 Image = 0; Image < Resource->DescriptorSet.DescriptorSetsCount; ++Image)
                    vk::DestroyDescriptorSets(Resource->DescriptorSet.DescriptorSets[Image].Pool,
                                              &Resource->DescriptorSet.DescriptorSets[Image].Handle,
                                              1);
                pfree(Resource->DescriptorSet.DescriptorSets);
            } break;
            
            case Resource_VertexBuffer:
            {
                vk::DestroyVmaBuffer(Resource->VertexBuffer.Buffer.Handle,
                                     Resource->VertexBuffer.Buffer.Memory);
            } break;
            
            case Resource_IndexBuffer:
            {
                vk::DestroyVmaBuffer(Resource->IndexBuffer.Buffer.Handle,
                                     Resource->IndexBuffer.Buffer.Memory);
            } break;
            
            case Resource_UniformBuffer:
            {
                u32 SwapchainImageCount = vk::GetSwapChainImageCount();
                for (u32 Buffer = 0; Buffer < SwapchainImageCount; ++Buffer)
                    vk::DestroyVmaBuffer(Resource->UniformBuffer.Buffers[Buffer].Handle,
                                         Resource->UniformBuffer.Buffers[Buffer].Memory);
                pfree(Resource->UniformBuffer.Buffers);
            } break;
            
            case Resource_DynamicUniformBuffer:
            {
                u32 SwapchainImageCount = vk::GetSwapChainImageCount();
                for (u32 Pool = 0; Pool < SwapchainImageCount; ++Pool)
                    mm::FreeDynUniformPool(&Resource->DynamicUniformBuffer.Pools[Pool]);
                pfree(Resource->DynamicUniformBuffer.Pools);
            } break;
            
            case Resource_Image:
            {
            } break;
            
            case Resource_Pipeline:
            {
                vk::DestroyPipelineLayout(Resource->Pipeline.Layout);
                vk::DestroyPipeline(Resource->Pipeline.Pipeline);
            } break;
            
            default: break;
        }
    }
    
    void UpdateUniform(resource_id_t Uniform, void *Data, u64 Size, u32 ImageIdx, u32 Offset)
    {
        resource *Resource = ResourceRegistry.Resources[Uniform];
        
        // NOTE(Dustin): Two Type of uniforms currently supported:
        // 1. Uniform
        // 2. Dynamic Uniform
        
        if (Resource->Type == Resource_UniformBuffer)
        {
            BufferParameters Buffer = Resource->UniformBuffer.Buffers[ImageIdx];
            
            if (Resource->UniformBuffer.PersistentlyMapped)
            {
                void *BufferPtr = Buffer.AllocationInfo.pMappedData;
                memcpy((char*)BufferPtr + Offset, Data, Size);
            }
            else
            {
                void *BufferPtr = nullptr;
                vk::VmaMap(&BufferPtr, Buffer.Memory);
                {
                    memcpy((char*)BufferPtr + Offset, Data, Size);
                }
                vk::VmaUnmap(Buffer.Memory);
            }
        }
        else if (Resource->Type == Resource_DynamicUniformBuffer)
        {
            mm::dyn_uniform_pool *Pool = &Resource->DynamicUniformBuffer.Pools[ImageIdx];
            AllocDynUniformPool(Pool, Data, Offset, Resource->UniformBuffer.PersistentlyMapped);
        }
    }
    
    // Updates the uniform memory for the global frame data for the shaders
    void UpdateGlobalFrameData(global_shader_data *Data, u32 ImageIndex)
    {
        resource *Resource = ResourceRegistry.Resources[ResourceRegistry.DefaultGlobalBuffer];
        
        BufferParameters Buffer = Resource->UniformBuffer.Buffers[ImageIndex];
        
        void *BufferPtr = Buffer.AllocationInfo.pMappedData;
        memcpy((char*)BufferPtr, Data, sizeof(global_shader_data));
    }
    
    void UpdateObjectFrameData(object_shader_data *Data, u32 Offset, u32 ImageIndex)
    {
        resource *Resource = ResourceRegistry.Resources[ResourceRegistry.ObjectDynamicBuffer];
        
        mm::dyn_uniform_pool *Pool = &Resource->DynamicUniformBuffer.Pools[ImageIndex];
        AllocDynUniformPool(Pool, Data, Offset, true);
    }
    
    // used for DynamicUniformBuffers to reset their internal allocator
    // Do not call directly! A Uniform reset should be done through the command
    // Gpu_ResetUniformBuffer
    void ResetDynamicUniformOffset(resource_id_t Uniform, u32 ImageIdx)
    {
        resource *Resource = ResourceRegistry.Resources[Uniform];
        
        mm::dyn_uniform_pool *Pool = &Resource->DynamicUniformBuffer.Pools[ImageIdx];
        Pool->Offset = 0;
    }
    
    resource GetResource(resource_id_t ResourceId)
    {
        resource Resource = *ResourceRegistry.Resources[ResourceId];
        return Resource;
    }
    
    dyn_uniform_template GetDynamicUniformTemplate(resource_id_t Uniform)
    {
        resource *Resource = ResourceRegistry.Resources[Uniform];
        
        mm::dyn_uniform_pool Pool = Resource->DynamicUniformBuffer.Pools[0];
        
        dyn_uniform_template Template = {};
        Template.Alignment   = Pool.Alignment;
        Template.ElementSize = Pool.ElementSize;
        Template.Size        = Pool.Buffer.Size;
        Template.Offset      = 0;
        
        return Template;
    }
    
    dyn_uniform_template GetDynamicObjectUniformTemplate()
    {
        return GetDynamicUniformTemplate(ResourceRegistry.ObjectDynamicBuffer);
    }
    
    
    i64 DynUniformGetNextOffset(dyn_uniform_template *DynUniformTemplate)
    {
        i64 Result = DynUniformTemplate->Offset;
        
        if (DynUniformTemplate->Offset + DynUniformTemplate->Alignment > DynUniformTemplate->Size)
        {
            mprinte("Requesting another offset but not enough space for another element!\n");
            Result = -1;
        }
        
        DynUniformTemplate->Offset += DynUniformTemplate->Alignment;
        
        return Result;
    }
    
    resource GetDefaultPipeline()
    {
        return *ResourceRegistry.Resources[ResourceRegistry.DefaultPipeline];
    }
    
    resource GetObjectDescriptorSet()
    {
        return *ResourceRegistry.Resources[ResourceRegistry.ObjectDescriptor];
    }
    
    resource GetObjectUniform()
    {
        return *ResourceRegistry.Resources[ResourceRegistry.ObjectDynamicBuffer];
    }
    
    resource GetDefaultFrameDescriptor()
    {
        return *ResourceRegistry.Resources[ResourceRegistry.DefaultGlobalDescriptor];
    }
    
    
}; // mresource