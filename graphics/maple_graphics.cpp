
#define EXTERN_GRAPHICS_API extern "C" GRAPHICS_API

globals *Core;

typedef enum cmd_type
{
    CmdType_BeginFrame,
    CmdType_EndFrame,
    
    CmdType_BindPipeline,
    CmdType_BindDescriptor,
    CmdType_Draw,
    
    CmdType_SetCamera,
    CmdType_UpdateObjectData,
    
    CmdType_Count,
    CmdType_Invalid,
} cmd_type;

typedef enum image_layout
{
    ImageLayout_Undefined,
    ImageLayout_TransferDst,
    ImageLayout_ShaderReadOnly,
} image_layout;

typedef struct cmd_bind_pipeline_info
{
    pipeline Pipeline;
} cmd_bind_pipeline_info;

typedef struct cmd_set_camera
{
    mat4 Projection;
    mat4 View;
} cmd_set_camera;

typedef struct cmd_set_object_world_data_info
{
    vec3       Position;
    vec3       Scale;
    quaternion Rotation;
} cmd_set_object_world_data_info;

typedef struct mp_command_pool
{
    void   *Ptr;
    memory  Pool;
    
    u64 LastCommandListSize;
    
    VkCommandPool Handle;
    
    // TODO(Dustin): Track all command lists allocated
    // from this pool. Introduce a Reset functions + have
    // free function clear all allocated command lists
} mp_command_pool;

typedef struct command_list_cmd
{
    cmd_type Type;
    u32      DataSize;
} command_list_cmd;

typedef struct mp_command_list
{
    command_pool AttachedPool;
    
    char *Start;
    char *End;
    char *Offset;
    
    bool  IsActive;
    u32   CommandCount;
    
    VkCommandBuffer *Handles;
    u32            CommandListCount;
} mp_command_list;

typedef struct mp_pipeline
{
    VkPipeline       Handle;
    VkPipelineLayout Layout;
    
    // Debug Pipelines
    VkPipeline       Wireframe;
    VkPipeline       NormalVis;
    
} mp_pipeline;

typedef struct mp_render_component
{
    buffer_parameters VertexBuffer;
    buffer_parameters IndexBuffer;
    
    bool              IsIndexed;
    VkIndexType       IndexType;
    u32               DrawCount;
} mp_render_component;

typedef struct mp_upload_buffer
{
    VkBuffer           Handle;
    VmaAllocation      Allocation;
    VmaAllocationInfo  AllocationInfo;
    upload_buffer_type Type; // Vertex or Index?
    u64                Size;
} mp_upload_buffer;

typedef struct mp_descriptor_layout
{
    VkDescriptorSetLayout Handle;
} mp_descriptor_layout;

typedef struct mp_descriptor_set
{
    VkDescriptorSet *Handles;
    u32              HandleCount;
    
    u32              Binding;
    u32              Set;
} mp_descriptor_set;

typedef struct mp_image
{
    VkImage           Handle;
    VkImageView       View;
    VkSampler         Sampler;
    VkFormat          Format;
    
    VmaAllocation     Memory;
    VmaAllocationInfo AllocationInfo;
    
    u32               Width;
    u32               Height;
    u32               MipLevels;
    
    image_layout CurrentLayout;
    
} mp_image;

void mp_command_pool_init(command_pool *CommandPool)
{
    u64 InitialMemory = _64KB;
    u64 StartingCommandListSize = _KB(1) * sizeof(command_list_cmd);
    
    command_pool pCommandPool = (command_pool)memory_alloc(Core->Memory, sizeof(mp_command_pool));
    pCommandPool->LastCommandListSize = StartingCommandListSize;
    pCommandPool->Ptr = memory_alloc(Core->Memory, InitialMemory);
    memory_init(&pCommandPool->Pool, InitialMemory, pCommandPool->Ptr);
    
    pCommandPool->Handle = Core->VkCore.CreateCommandPool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                                          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    
    *CommandPool = pCommandPool;
}

void mp_command_pool_free(command_pool *CommandPool)
{
    Core->VkCore.DestroyCommandPool((*CommandPool)->Handle);
    
    memory_free(&(*CommandPool)->Pool);
    memory_release(Core->Memory, (*CommandPool)->Ptr);
    memory_release(Core->Memory, *CommandPool);
    *CommandPool = NULL;
}

void mp_command_list_init(command_list *CommandList, command_pool CommandPool)
{
    command_list pCommandList  = (command_list)memory_alloc(&CommandPool->Pool, sizeof(mp_command_list));
    pCommandList->AttachedPool = CommandPool;
    pCommandList->Start        = (char*)memory_alloc(&pCommandList->AttachedPool->Pool, CommandPool->LastCommandListSize);
    pCommandList->End          = (char*)pCommandList->Start + CommandPool->LastCommandListSize;
    pCommandList->Offset       = (char*)pCommandList->Start;
    pCommandList->IsActive     = false;
    pCommandList->CommandCount = 0;
    
    pCommandList->CommandListCount = Core->VkCore.GetSwapChainImageCount();
    pCommandList->Handles = (VkCommandBuffer*)memory_alloc(Core->Memory, pCommandList->CommandListCount);
    Core->VkCore.CreateCommandBuffers(pCommandList->AttachedPool->Handle,
                                      VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                      pCommandList->CommandListCount,
                                      pCommandList->Handles);
    
    *CommandList = pCommandList;
}

void mp_command_list_free(command_list *CommandList)
{
    memory_release(Core->Memory, (*CommandList)->Handles);
    memory_release(&(*CommandList)->AttachedPool->Pool, (*CommandList)->Start);
    (*CommandList)->Start        = NULL;
    (*CommandList)->End          = NULL;
    (*CommandList)->Offset       = NULL;
    
    command_pool CommandPool = (*CommandList)->AttachedPool;
    (*CommandList)->AttachedPool = NULL;
    
    memory_release(&CommandPool->Pool, (*CommandList));
    
    *CommandList = NULL;
}


// Memory Layout for a Single Command List Cmd
// | Type , DataSize | Data
//
// alloc(CommandPool, sizeof(command_list_cmd) + DataSize)
//
// char *Offset = (char*)CommandList->Start;
// for each element in commandlist
//    command_list_cmd *Cmd = (command_list_cmd*)Offset;
//    u32 *Size = (u32*)(Offset + sizeof(cmd_type));
//    void *Data = Offset + sizeof(command_list_cmd);
//
//    Offset += sizeof(cmd_type) + sizeof(u32) + *Size;
//

void mp_command_list_add(command_list CommandList, cmd_type Type, u32 DataSize, void *Data)
{
    u32 NeededMemory = sizeof(command_list_cmd) + DataSize;
    
    if (CommandList->Offset + NeededMemory > CommandList->End)
    {
        // TODO(Dustin): Resize the CommandList!
    }
    
    char *Ptr = CommandList->Offset;
    CommandList->Offset += NeededMemory;
    
    command_list_cmd *Cmd = (command_list_cmd*)Ptr;
    Cmd->Type = Type;
    Cmd->DataSize = DataSize;
    memcpy(Ptr + sizeof(command_list_cmd), Data, DataSize);
    
    CommandList->CommandCount ++;
}

void mp_command_list_execute(command_list CommandList)
{
    if (Core->Renderer->ActiveCommandBuffer)
    {
        VkCommandBuffer *ActiveCommandBuffer = Core->Renderer->ActiveCommandBuffer;
        
        char *Offset = CommandList->Start;
        for (u32 i = 0; i < CommandList->CommandCount; ++i)
        {
            command_list_cmd *Cmd = (command_list_cmd*)Offset;
            void *Data = Offset + sizeof(command_list_cmd);
            
            // TODO(Dustin): Big ol' switch state for processing various commands
            switch (Cmd->Type)
            {
                case CmdType_BeginFrame:
                {
                } break;
                
                case CmdType_EndFrame:
                {
                } break;
                
                case CmdType_BindPipeline:
                {
                    cmd_bind_pipeline_info *PipelineInfo = (cmd_bind_pipeline_info*)Data;
                    
                    if (Core->Renderer->RenderMode & RenderMode_Solid)
                    {
                        Core->VkCore.BindPipeline(*ActiveCommandBuffer, PipelineInfo->Pipeline->Handle);
                    }
                    else if (Core->Renderer->RenderMode & RenderMode_Wireframe)
                    {
                        Core->VkCore.BindPipeline(*ActiveCommandBuffer, PipelineInfo->Pipeline->Wireframe);
                    }
                    
                    Core->Renderer->ActivePipeline = PipelineInfo->Pipeline;
                    Core->VkCore.BindDescriptorSets(*ActiveCommandBuffer,
                                                    Core->Renderer->ActivePipeline->Layout,
                                                    0,
                                                    1,
                                                    &Core->Renderer->GlobalShaderData.DescriptorSets[Core->Renderer->CurrentImageIndex],
                                                    0, NULL);
                } break;
                
                case CmdType_BindDescriptor:
                {
                    descriptor_set Set = (descriptor_set)Data;
                    
                    Core->VkCore.BindDescriptorSets(*ActiveCommandBuffer,
                                                    Core->Renderer->ActivePipeline->Layout,
                                                    Set->Set,
                                                    1,
                                                    &Set->Handles[Core->Renderer->CurrentImageIndex],
                                                    0, NULL);
                } break;
                
                case CmdType_Draw:
                {
                    render_component RenderComponent = (render_component)Data;
                    
                    // Bind Vertex Buffers
                    {
                        u32 BuffersCount = 1;
                        
                        VkBuffer Buffers[1] = {
                            RenderComponent->VertexBuffer.Handle,
                        };
                        
                        u64 BufferOffsets[1] = {
                            0
                        };
                        
                        Core->VkCore.BindVertexBuffers(*ActiveCommandBuffer, 0, BuffersCount,
                                                       Buffers, BufferOffsets);
                    }
                    
                    if (RenderComponent->IsIndexed)
                    {
                        // Bind Index Buffers
                        Core->VkCore.BindIndexBuffer(*ActiveCommandBuffer, 
                                                     RenderComponent->IndexBuffer.Handle, 
                                                     0, 
                                                     RenderComponent->IndexType);
                        
                        Core->VkCore.DrawIndexed(*ActiveCommandBuffer, RenderComponent->DrawCount, 1, 0, 0, 0);
                    }
                    else
                    {
                        Core->VkCore.Draw(*ActiveCommandBuffer, RenderComponent->DrawCount, 1, 0, 0);
                    }
                    
                } break;
                
                case CmdType_SetCamera:
                {
                    camera_data *CameraData = (camera_data*)Data;
                    
                    Core->Renderer->ActiveCamera.Projection = CameraData->Projection;
                    Core->Renderer->ActiveCamera.View       = CameraData->View;
                    
                    mp_uniform_buffer_update(&Core->Renderer->GlobalShaderData.Buffer,
                                             &Core->Renderer->ActiveCamera,
                                             sizeof(camera_data),
                                             0);
                    
                    // NOTE(Dustin): The descriptor is bound with the pipeline since it is a 
                    // global descriptor
                } break;
                
                case CmdType_UpdateObjectData:
                {
                    cmd_set_object_world_data_info *ObjectData = (cmd_set_object_world_data_info*)Data;
                    
                    // TODO(Dustin): Set up real model matrix
                    mat4 Translation = translate(ObjectData->Position);
                    mat4 Scale       = scale(ObjectData->Scale.x, ObjectData->Scale.y, ObjectData->Scale.z);
                    mat4 Rotation    = quaternion_get_rotation_matrix(ObjectData->Rotation);
                    
                    mat4 Model = mat4_diag(1.0f);
                    Model = mat4_mul(Model, Scale);
                    Model = mat4_mul(Model, Rotation);
                    Model = mat4_mul(Model, Translation);
                    
                    u32 Offset = mp_dynamic_uniform_buffer_alloc(&Core->Renderer->ObjectDataBuffer.Buffer,
                                                                 &Model,
                                                                 sizeof(mat4));
                    
                    Core->VkCore.BindDescriptorSets(*ActiveCommandBuffer,
                                                    Core->Renderer->ActivePipeline->Layout,
                                                    1,
                                                    1,
                                                    &Core->Renderer->ObjectDataBuffer.DescriptorSets[Core->Renderer->CurrentImageIndex],
                                                    1, &Offset);
                } break;
                
            }
            
            Offset += sizeof(command_list_cmd) + Cmd->DataSize;
        }
    }
    else
    {
        mprinte("Attempting to execute a Command List without an active Command Buffer. Have you called \"begin_frame\"?\n");
    }
    
    CommandList->CommandCount = 0;
    CommandList->Offset = CommandList->Start;
}

INITIALIZE_GRAPHICS(initialize_graphics)
{
    u32 MemorySize = _1MB;
    
    // Initialize memory
    void *PlatformMemory = PlatformRequestMemory(MemorySize);
    
    memory Memory = {0};
    memory_init(&Memory, MemorySize, PlatformMemory);
    
    memory *pMemory = (memory*)memory_alloc(&Memory, sizeof(memory));
    *pMemory = Memory;
    
    Core = (globals*)memory_alloc(pMemory, sizeof(globals));
    Core->Memory = pMemory;
    
    PlatformSetClientWindow((platform_window*)CreateInfo->Window);
    
    // Initialize Vulkan
    mprint("Initializing Vulkan...\n");
    Core->VkCore = {};
    Core->VkCore.Init();
    
    // Initialize the Renderer
    mprint("Initializing the Renderer...\n");
    Core->Renderer = palloc<renderer>();
    *Core->Renderer = {};
    renderer_init(Core->Renderer);
}

SHUTDOWN_GRAPHICS(shutdown_graphics)
{
    renderer_free(Core->Renderer);
    pfree(Core->Renderer);
    
    Core->VkCore.Shutdown();
    
    memory Memory = *Core->Memory;
    void *MemoryPtr = Memory.Start;;
    
    memory_release(&Memory, Core->Memory);
    Core->Memory = NULL;
    
    memory_release(&Memory, Core);
    Core = NULL;
    
    PlatformReleaseMemory(MemoryPtr, 0);
}

BEGIN_FRAME(begin_frame)
{
    renderer_begin_frame();
}

END_FRAME(end_frame)
{
#if 0
    if (EndFrameInfo->CommandListCount <= 3)
    {
        VkCommandBuffer CommandBuffers[3];
        for (u32 i = 0; i < EndFrameInfo->CommandListCount; ++i)
        {
            CommandBuffers[i] = EndFrameInfo->CommandList[i]->Handles[Renderer.CurrentImageIndex];
        }
        
        Core->VkCore.EndFrame(Renderer.CurrentImageIndex, CommandBuffers, EndFrameInfo->CommandListCount);
    }
    else
    {
        VkCommandBuffer *CommandBuffers = (VkCommandBuffer*)memory_alloc(Core->Memory, 
                                                                         sizeof(VkCommandBuffer) * EndFrameInfo->CommandListCount);
        for (u32 i = 0; i < EndFrameInfo->CommandListCount; ++i)
        {
            CommandBuffers[i] = EndFrameInfo->CommandList[i]->Handles[Renderer.CurrentImageIndex];
        }
        
        Core->VkCore.EndFrame(Renderer.CurrentImageIndex, CommandBuffers, EndFrameInfo->CommandListCount);
        
        memory_release(Core->Memory, CommandBuffers);
    }
#else
    
    renderer_end_frame();
    
#endif
}

WAIT_FOR_LAST_FRAME(wait_for_last_frame)
{
    Core->VkCore.Idle();
}

/*

command_pool CommandPool;
command_list CommandList;

command_pool_create_info Info = {};
Info->CommandPool = &CommandPool;
create_command_pool(&info);

command_list_create_info Info = {};
Info->CommandList = CommandList;
Info->CommandPool = CommandPool;
create_command_list(&info);

*/

CREATE_COMMAND_LIST(create_command_list)
{
    mp_command_list_init(CreateInfo->CommandList, CreateInfo->CommandPool);
}

FREE_COMMAND_LIST(free_command_list)
{
    mp_command_list_free(CommandList);
}

EXECUTE_COMMAND_LIST(execute_command_list)
{
    mp_command_list_execute(CommandList);
}

CREATE_COMMAND_POOL(create_command_pool)
{
    mp_command_pool_init(CreateInfo->CommandPool);
}

FREE_COMMAND_POOL(free_command_pool)
{
    mp_command_pool_free(CommandPool);
}

file_internal void LoadShader(char *ShaderFileName,
                              VkShaderStageFlagBits ShaderStage,
                              VkShaderModule &ShaderModule,
                              VkPipelineShaderStageCreateInfo &ShaderStageInfo)
{
    file_t ShaderFile = PlatformLoadFile(ShaderFileName);
    
    ShaderModule = Core->VkCore.CreateShaderModule((u32*)(GetFileBuffer(ShaderFile)), 
                                                   PlatformGetFileSize(ShaderFile));
    
    ShaderStageInfo = {};
    ShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStageInfo.stage  = ShaderStage;
    ShaderStageInfo.module = ShaderModule;
    ShaderStageInfo.pName  = "main";
    
    PlatformCloseFile(ShaderFile);
}

CREATE_PIPELINE(create_pipeline) 
{
    pipeline pPipeline = (pipeline)memory_alloc(Core->Memory, sizeof(mp_pipeline));
    
    VkShaderModule ShaderModules[5];
    VkPipelineShaderStageCreateInfo ShaderStages[5];
    u32 ShaderStageCount = 0;
    
    // Load Vertex Shader
    if (PipelineInfo->VertexShader)
    {
        LoadShader(PipelineInfo->VertexShader,
                   VK_SHADER_STAGE_VERTEX_BIT ,
                   ShaderModules[ShaderStageCount],
                   ShaderStages[ShaderStageCount]);
        ShaderStageCount++;
    }
    
    // Load Fragment Shader
    if (PipelineInfo->FragmentShader)
    {
        LoadShader(PipelineInfo->FragmentShader,
                   VK_SHADER_STAGE_FRAGMENT_BIT ,
                   ShaderModules[ShaderStageCount],
                   ShaderStages[ShaderStageCount]);
        ShaderStageCount++;
    }
    
    // Load Geometry Shader
    if (PipelineInfo->GeometryShader)
    {
        LoadShader(PipelineInfo->GeometryShader,
                   VK_SHADER_STAGE_GEOMETRY_BIT ,
                   ShaderModules[ShaderStageCount],
                   ShaderStages[ShaderStageCount]);
        ShaderStageCount++;
    }
    
    // Load Tess Control Shader
    if (PipelineInfo->TessControlShader)
    {
        LoadShader(PipelineInfo->TessControlShader,
                   VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
                   ShaderModules[ShaderStageCount],
                   ShaderStages[ShaderStageCount]);
        ShaderStageCount++;
    }
    
    // Load Tess Eval Shader
    if (PipelineInfo->TessEvalShader)
    {
        LoadShader(PipelineInfo->TessEvalShader,
                   VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                   ShaderModules[ShaderStageCount],
                   ShaderStages[ShaderStageCount]);
        ShaderStageCount++;
    }
    
    VkPipelineInputAssemblyStateCreateInfo InputAssembly = {};
    InputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssembly.topology               = PipelineInfo->Topology; // was Triangle_List
    InputAssembly.primitiveRestartEnable = VK_FALSE;
    
    VkPipelineViewportStateCreateInfo ViewportState = {};
    ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportState.viewportCount = PipelineInfo->ViewportCount;
    ViewportState.pViewports    = PipelineInfo->Viewport;
    ViewportState.scissorCount  = PipelineInfo->ScissorCount;
    ViewportState.pScissors     = PipelineInfo->Scissor;
    
    // Rasterizer
    VkPipelineRasterizationStateCreateInfo Rasterizer = {};
    Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    Rasterizer.depthClampEnable        = VK_FALSE;
    Rasterizer.rasterizerDiscardEnable = VK_FALSE;
    Rasterizer.polygonMode             = PipelineInfo->PolygonMode;
    Rasterizer.lineWidth               = PipelineInfo->LineWidth;
    Rasterizer.frontFace               = PipelineInfo->FrontFace;
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
    
    if (PipelineInfo->HasMultisampling)
    {
        Multisampling.rasterizationSamples = PipelineInfo->MuliSampleSamples;
    }
    else
    {
        Multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    }
    
    // Depth/Stencil Testing - not right now
    VkPipelineDepthStencilStateCreateInfo DepthStencil = {};
    DepthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencil.depthTestEnable       = VK_TRUE;
    DepthStencil.depthWriteEnable      = VK_TRUE;
    DepthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
    DepthStencil.depthBoundsTestEnable = VK_FALSE;
    DepthStencil.stencilTestEnable     = VK_FALSE;
    
    // Color Blending
    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo ColorBlending = {};
    ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlending.logicOpEnable     = VK_FALSE;
    ColorBlending.logicOp           = VK_LOGIC_OP_COPY;
    ColorBlending.attachmentCount   = 1;
    ColorBlending.pAttachments      = &ColorBlendAttachment;
    ColorBlending.blendConstants[0] = 0.0f;
    ColorBlending.blendConstants[1] = 0.0f;
    ColorBlending.blendConstants[2] = 0.0f;
    ColorBlending.blendConstants[3] = 0.0f;
    
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
    
    // Create the pipeline layout
    VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
    PipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    // The descriptors...needs to append descriptors the renderer handles internally.
    // 1. GlobalShaderData DescriptorLayout
    // 2. ObjectDataBuffer DescriptorLayout
    u32 LayoutCount = PipelineInfo->DescriptorLayoutsCount + 2;
    VkDescriptorSetLayout *Layouts = (VkDescriptorSetLayout*)memory_alloc(Core->Memory, 
                                                                          LayoutCount * sizeof(VkDescriptorSetLayout));
    Layouts[0] = Core->Renderer->GlobalShaderData.DescriptorLayout;
    Layouts[1] = Core->Renderer->ObjectDataBuffer.DescriptorLayout;
    
    for (u32 i = 0; i < PipelineInfo->DescriptorLayoutsCount; ++i)
    {
        Layouts[i + 2] = PipelineInfo->DescriptorLayouts[i]->Handle;
    }
    
    PipelineLayoutInfo.setLayoutCount         = LayoutCount;
    PipelineLayoutInfo.pSetLayouts            = Layouts;
    PipelineLayoutInfo.pushConstantRangeCount = PipelineInfo->PushConstantsCount;
    PipelineLayoutInfo.pPushConstantRanges    = PipelineInfo->PushConstants;
    
    pPipeline->Layout = Core->VkCore.CreatePipelineLayout(PipelineLayoutInfo);
    
    // create the pipeline
    VkGraphicsPipelineCreateInfo PipelineCreateInfo = {};
    PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.stageCount          = ShaderStageCount;
    PipelineCreateInfo.pStages             = ShaderStages;
    PipelineCreateInfo.pVertexInputState   = &PipelineInfo->VertexInputInfo;
    PipelineCreateInfo.pInputAssemblyState = &InputAssembly;
    PipelineCreateInfo.pViewportState      = &ViewportState;
    PipelineCreateInfo.pRasterizationState = &Rasterizer;
    PipelineCreateInfo.pMultisampleState   = &Multisampling;
    PipelineCreateInfo.pDepthStencilState  = &DepthStencil;
    PipelineCreateInfo.pColorBlendState    = &ColorBlending;
    PipelineCreateInfo.pDynamicState       = &DynamicStateInfo;
    PipelineCreateInfo.layout              = pPipeline->Layout;
    
#if 0
    PipelineCreateInfo.renderPass          = PipelineInfo->RenderPass;
#else
    PipelineCreateInfo.renderPass          = Core->Renderer->PrimaryRenderPass;
#endif
    
    PipelineCreateInfo.subpass             = 0;
    PipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
    PipelineCreateInfo.basePipelineIndex   = -1;
    
    pPipeline->Handle = Core->VkCore.CreatePipeline(PipelineCreateInfo);
    
    //~ Create Wireframe Visualization
    
    Rasterizer = {};
    Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    Rasterizer.depthClampEnable        = VK_FALSE;
    Rasterizer.rasterizerDiscardEnable = VK_FALSE;
    Rasterizer.polygonMode             = VK_POLYGON_MODE_LINE;
    Rasterizer.lineWidth               = PipelineInfo->LineWidth;
    Rasterizer.frontFace               = PipelineInfo->FrontFace;
    Rasterizer.depthBiasEnable         = VK_FALSE;
    Rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    Rasterizer.depthBiasClamp          = 0.0f; // Optional
    Rasterizer.depthBiasSlopeFactor    = 0.0f; // Optional
    
    PipelineCreateInfo = {};
    PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.stageCount          = ShaderStageCount;
    PipelineCreateInfo.pStages             = ShaderStages;
    PipelineCreateInfo.pVertexInputState   = &PipelineInfo->VertexInputInfo;
    PipelineCreateInfo.pInputAssemblyState = &InputAssembly;
    PipelineCreateInfo.pViewportState      = &ViewportState;
    PipelineCreateInfo.pRasterizationState = &Rasterizer;
    PipelineCreateInfo.pMultisampleState   = &Multisampling;
    PipelineCreateInfo.pDepthStencilState  = &DepthStencil;
    PipelineCreateInfo.pColorBlendState    = &ColorBlending;
    PipelineCreateInfo.pDynamicState       = &DynamicStateInfo;
    PipelineCreateInfo.layout              = pPipeline->Layout;
    PipelineCreateInfo.renderPass          = Core->Renderer->PrimaryRenderPass;
    
    pPipeline->Wireframe = Core->VkCore.CreatePipeline(PipelineCreateInfo);
    
    //~ Create the Normal Visualization Pipeline
    if (0) {
        // HACK(Dustin): Assume only Vertex and Fragment
        LoadShader("data/shaders/normal_vis.geom.spv",
                   VK_SHADER_STAGE_GEOMETRY_BIT,
                   ShaderModules[2],
                   ShaderStages[2]);
        ShaderStageCount++;
        
        Rasterizer = {};
        Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        Rasterizer.depthClampEnable        = VK_FALSE;
        Rasterizer.rasterizerDiscardEnable = VK_FALSE;
        Rasterizer.polygonMode             = VK_POLYGON_MODE_LINE;
        Rasterizer.lineWidth               = PipelineInfo->LineWidth;
        Rasterizer.frontFace               = PipelineInfo->FrontFace;
        Rasterizer.depthBiasEnable         = VK_FALSE;
        Rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        Rasterizer.depthBiasClamp          = 0.0f; // Optional
        Rasterizer.depthBiasSlopeFactor    = 0.0f; // Optional
        
        PipelineCreateInfo = {};
        PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        PipelineCreateInfo.stageCount          = ShaderStageCount;
        PipelineCreateInfo.pStages             = ShaderStages;
        PipelineCreateInfo.pVertexInputState   = &PipelineInfo->VertexInputInfo;
        PipelineCreateInfo.pInputAssemblyState = &InputAssembly;
        PipelineCreateInfo.pViewportState      = &ViewportState;
        PipelineCreateInfo.pRasterizationState = &Rasterizer;
        PipelineCreateInfo.pMultisampleState   = &Multisampling;
        PipelineCreateInfo.pDepthStencilState  = &DepthStencil;
        PipelineCreateInfo.pColorBlendState    = &ColorBlending;
        PipelineCreateInfo.pDynamicState       = &DynamicStateInfo;
        PipelineCreateInfo.layout              = pPipeline->Layout;
        PipelineCreateInfo.renderPass          = Core->Renderer->PrimaryRenderPass;
        
        pPipeline->NormalVis = Core->VkCore.CreatePipeline(PipelineCreateInfo);
        
    }
    
    //~ Destroy the shader stage modules
    for (u32 Shader = 0; Shader < ShaderStageCount; Shader++)
    {
        Core->VkCore.DestroyShaderModule(ShaderModules[Shader]);
    }
    
    memory_release(Core->Memory, Layouts);
    
    *Pipeline = pPipeline;
}

FREE_PIPELINE(free_pipeline) 
{
    Core->VkCore.DestroyPipelineLayout((*Pipeline)->Layout);
    Core->VkCore.DestroyPipeline((*Pipeline)->Handle);
    Core->VkCore.DestroyPipeline((*Pipeline)->Wireframe);
    Core->VkCore.DestroyPipeline((*Pipeline)->NormalVis);
    
    memory_release(Core->Memory, (*Pipeline));
    *Pipeline = NULL;
}

CREATE_RENDER_COMPONENT(create_render_component)
{
    render_component Result = (render_component)memory_alloc(Core->Memory, sizeof(mp_render_component));
    
    VkBufferCreateInfo VertexBufferInfo = {};
    VertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    VertexBufferInfo.size  = RenderInfo->VertexCount * RenderInfo->VertexStride;
    VertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    VmaAllocationCreateInfo VertexAllocInfo = {};
    VertexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    Core->VkCore.CreateVmaBufferWithStaging(VertexBufferInfo,
                                            VertexAllocInfo,
                                            Core->Renderer->CommandPool,
                                            Result->VertexBuffer.Handle,
                                            Result->VertexBuffer.Memory,
                                            RenderInfo->VertexData,
                                            VertexBufferInfo.size);
    
    Result->VertexBuffer.Size = VertexBufferInfo.size;
    
    if (RenderInfo->HasIndices)
    {
        Result->IsIndexed = true;
        
        if (RenderInfo->IndexStride == 2)
        {
            Result->IndexType = VK_INDEX_TYPE_UINT16;
        }
        else if (RenderInfo->IndexStride == 4.)
        {
            Result->IndexType = VK_INDEX_TYPE_UINT32;
        }
        else
        {
            mprinte("Attempting to create index buffer with stride %ld, but this is invalid. Defaulting to 32bit index type.\n", RenderInfo->IndexStride);
            Result->IndexType = VK_INDEX_TYPE_UINT32;
        }
        
        VkBufferCreateInfo IndexBufferInfo = {};
        IndexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        IndexBufferInfo.size  = RenderInfo->IndexCount * RenderInfo->IndexStride;
        IndexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        
        VmaAllocationCreateInfo IndexAllocInfo = {};
        IndexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        Core->VkCore.CreateVmaBufferWithStaging(IndexBufferInfo,
                                                IndexAllocInfo,
                                                Core->Renderer->CommandPool,
                                                Result->IndexBuffer.Handle,
                                                Result->IndexBuffer.Memory,
                                                RenderInfo->IndexData,
                                                IndexBufferInfo.size);
        
        Result->IndexBuffer.Size = IndexBufferInfo.size;
        Result->DrawCount = RenderInfo->IndexCount;
    }
    else
    {
        Result->IsIndexed = false;
        Result->DrawCount = RenderInfo->VertexCount;
    }
    
    
    *RenderComponent = Result;
}

FREE_RENDER_COMPONENT(free_render_component)
{
    Core->VkCore.DestroyVmaBuffer((*RenderComponent)->VertexBuffer.Handle,
                                  (*RenderComponent)->VertexBuffer.Memory);
    
    Core->VkCore.DestroyVmaBuffer((*RenderComponent)->IndexBuffer.Handle,
                                  (*RenderComponent)->IndexBuffer.Memory);
    
    memory_release(Core->Memory, *RenderComponent);
    *RenderComponent = NULL;
}

SET_RENDER_COMPONENT_INFO(set_render_component_info)
{
    RenderComponent->IsIndexed = IsIndexed;
    RenderComponent->IndexType = IndexType;
    RenderComponent->DrawCount = DrawCount;
}

CREATE_UPLOAD_BUFFER(create_upload_buffer)
{
    upload_buffer Result = (upload_buffer)memory_alloc(Core->Memory, sizeof(mp_upload_buffer));
    
    Result->Type = BufferType;
    Result->Size = BufferSize;
    Result->AllocationInfo = {};
    
    VkBufferCreateInfo StagingBufferInfo = {};
    StagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    StagingBufferInfo.size  = Result->Size;
    StagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    
    VmaAllocationCreateInfo AllocInfo = {};
    AllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    AllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    Core->VkCore.CreateVmaBuffer(StagingBufferInfo,
                                 AllocInfo,
                                 Result->Handle,
                                 Result->Allocation,
                                 Result->AllocationInfo);
    
    *Buffer = Result;
}

RESIZE_UPLOAD_BUFFER(resize_upload_buffer)
{
    if (NewSize > Buffer->Size)
    {
        VkBuffer           Handle;
        VmaAllocation      Allocation;
        VmaAllocationInfo  AllocationInfo;
        
        VkBufferCreateInfo StagingBufferInfo = {};
        StagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        StagingBufferInfo.size  = NewSize;
        StagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        
        VmaAllocationCreateInfo AllocInfo = {};
        AllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        AllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        
        Core->VkCore.CreateVmaBuffer(StagingBufferInfo,
                                     AllocInfo,
                                     Handle,
                                     Allocation,
                                     AllocationInfo);
        
        char *New = (char*)AllocationInfo.pMappedData;
        char *Old = (char*)Buffer->AllocationInfo.pMappedData;
        //memcpy(New, Old, Buffer->Size);
        
        Core->VkCore.Idle();
        Core->VkCore.DestroyVmaBuffer(Buffer->Handle, Buffer->Allocation);
        
        Buffer->Handle         = Handle;
        Buffer->Allocation     = Allocation;
        Buffer->AllocationInfo = AllocationInfo;
        Buffer->Size           = NewSize;
    }
}

COPY_UPLOAD_BUFFER(copy_upload_buffer)
{
    // Make sure the last frame is done rendering
    // TODO(Dustin): Find a better way to do this so you don't have to vk::idle
    Core->VkCore.Idle();
    
    if (UploadBuffer->Type == UploadBuffer_Vertex)
    {
        // First make sure the vertex buffer is large enough
        if (RenderComponent->VertexBuffer.Size < UploadBuffer->Size)
        {
            VkBuffer NewVertexBuffer;
            VmaAllocation NewVmaAllocation;
            VmaAllocationInfo NewVmaAllocationInfo;
            
            VkBufferCreateInfo VertexBufferInfo = {};
            VertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            VertexBufferInfo.size  = UploadBuffer->Size;
            VertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            
            VmaAllocationCreateInfo VertexAllocInfo = {};
            VertexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            
            Core->VkCore.CreateVmaBuffer(VertexBufferInfo,
                                         VertexAllocInfo,
                                         NewVertexBuffer,
                                         NewVmaAllocation,
                                         NewVmaAllocationInfo);
            
            Core->VkCore.CopyBuffer(Core->Renderer->CommandPool, 
                                    UploadBuffer->Handle,
                                    NewVertexBuffer,
                                    UploadBuffer->Size);
            
            
            Core->VkCore.DestroyVmaBuffer(RenderComponent->VertexBuffer.Handle, 
                                          RenderComponent->VertexBuffer.Memory);
            
            RenderComponent->VertexBuffer.Handle = NewVertexBuffer;
            RenderComponent->VertexBuffer.Memory = NewVmaAllocation;
            RenderComponent->VertexBuffer.Size   = UploadBuffer->Size;
            RenderComponent->VertexBuffer.AllocationInfo = NewVmaAllocationInfo;
        }
        else
        {
            Core->VkCore.CopyBuffer(Core->Renderer->CommandPool, 
                                    UploadBuffer->Handle,
                                    RenderComponent->VertexBuffer.Handle,
                                    UploadBuffer->Size);
        }
    }
    else if (UploadBuffer->Type == UploadBuffer_Index)
    {
        // First make sure the index buffer is large enough
        if (RenderComponent->IndexBuffer.Size < UploadBuffer->Size)
        {
            VkBuffer NewIndexBuffer;
            VmaAllocation NewVmaAllocation;
            VmaAllocationInfo NewVmaAllocationInfo;
            
            VkBufferCreateInfo IndexBufferInfo = {};
            IndexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            IndexBufferInfo.size  = UploadBuffer->Size;
            IndexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            
            VmaAllocationCreateInfo IndexAllocInfo = {};
            IndexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            
            Core->VkCore.CreateVmaBuffer(IndexBufferInfo,
                                         IndexAllocInfo,
                                         NewIndexBuffer,
                                         NewVmaAllocation,
                                         NewVmaAllocationInfo);
            
            Core->VkCore.DestroyVmaBuffer(RenderComponent->IndexBuffer.Handle, 
                                          RenderComponent->IndexBuffer.Memory);
            
            RenderComponent->IndexBuffer.Handle = NewIndexBuffer;
            RenderComponent->IndexBuffer.Memory = NewVmaAllocation;
            RenderComponent->IndexBuffer.Size   = UploadBuffer->Size;
            RenderComponent->IndexBuffer.AllocationInfo = NewVmaAllocationInfo;
            
            Core->VkCore.CopyBuffer(Core->Renderer->CommandPool, 
                                    UploadBuffer->Handle,
                                    RenderComponent->IndexBuffer.Handle,
                                    UploadBuffer->Size);
            
        }
        else
        {
            Core->VkCore.CopyBuffer(Core->Renderer->CommandPool, 
                                    UploadBuffer->Handle,
                                    RenderComponent->IndexBuffer.Handle,
                                    UploadBuffer->Size);
        }
    }
}

FREE_UPLOAD_BUFFER(free_upload_buffer)
{
    Core->VkCore.DestroyVmaBuffer((*Buffer)->Handle, 
                                  (*Buffer)->Allocation);
    
    memory_release(Core->Memory, *Buffer);
    *Buffer = NULL;
}

UPDATE_UPLOAD_BUFFER(update_upload_buffer)
{
    // Resize the buffer
    if (Offset + Size > UploadBuffer->Size)
    {
        resize_upload_buffer(UploadBuffer, (UploadBuffer->Size > 0) ? UploadBuffer->Size * 2 : Size);
    }
    
    memcpy((char*)UploadBuffer->AllocationInfo.pMappedData + Offset, Data, Size);
}

MAP_UPLOAD_BUFFER(map_upload_buffer)
{
    *Ptr = (char*)UploadBuffer->AllocationInfo.pMappedData + Offset;
}

UNMAP_UPLOAD_BUFFER(unmap_upload_buffer)
{
    *Ptr = NULL;
}

GET_UPLOAD_BUFFER_INFO(get_upload_buffer_info)
{
    upload_buffer_info Result = {};
    
    Result.Size = UploadBuffer->Size;
    Result.Type = UploadBuffer->Type;
    
    return Result;
}


CREATE_IMAGE(create_image)
{
    image Result = (image)memory_alloc(Core->Memory, sizeof(mp_image));
    
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = ImageInfo->Width;
    imageInfo.extent.height = ImageInfo->Height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = ImageInfo->MipLevels;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = ImageInfo->ImageFormat;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags         = 0; // Optional
    
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    // Create the image
    Core->VkCore.CreateVmaImage(imageInfo, alloc_info,
                                Result->Handle,
                                Result->Memory,
                                Result->AllocationInfo);
    
    Core->VkCore.TransitionImageLayout(Core->Renderer->CommandPool,
                                       Result->Handle,
                                       ImageInfo->ImageFormat,
                                       VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       ImageInfo->MipLevels);
    
    Result->CurrentLayout = ImageLayout_TransferDst;
    Result->Width  = ImageInfo->Width;
    Result->Height = ImageInfo->Height;
    
    // Create the Image View
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = Result->Handle;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = ImageInfo->ImageFormat;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = ImageInfo->MipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;
    
    Result->View = Core->VkCore.CreateImageView(viewInfo);
    
    // Create the Image Sampler
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = ImageInfo->MagFilter;
    samplerInfo.minFilter               = ImageInfo->MinFilter;
    samplerInfo.addressModeU            = ImageInfo->AddressModeU;
    samplerInfo.addressModeV            = ImageInfo->AddressModeV;
    samplerInfo.addressModeW            = ImageInfo->AddressModeW;
    samplerInfo.anisotropyEnable        = VK_TRUE;
    samplerInfo.maxAnisotropy           = 16;
    samplerInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias              = 0.0f;
    samplerInfo.minLod                  = 0.0f;
    samplerInfo.maxLod                  = (float)ImageInfo->MipLevels;
    
    Result->Sampler = Core->VkCore.CreateImageSampler(samplerInfo);
    
    Result->Format = ImageInfo->ImageFormat;
    Result->MipLevels = ImageInfo->MipLevels;
    
    *Image = Result;
}

RESIZE_IMAGE(resize_image)
{
    Core->VkCore.Idle();
    
    Core->VkCore.DestroyVmaImage(Image->Handle, Image->Memory);
    
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = Width;
    imageInfo.extent.height = Height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = Image->MipLevels;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = Image->Format;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags         = 0; // Optional
    
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    Core->VkCore.CreateVmaImage(imageInfo, alloc_info,
                                Image->Handle,
                                Image->Memory,
                                Image->AllocationInfo);
    
    Core->VkCore.TransitionImageLayout(Core->Renderer->CommandPool,
                                       Image->Handle,
                                       Image->Format,
                                       VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       Image->MipLevels);
    
    
    Core->VkCore.DestroyImageView(Image->View);
    
    // Create the Image View
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = Image->Handle;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = Image->Format;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = Image->MipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;
    
    Image->View = Core->VkCore.CreateImageView(viewInfo);
    
}

FREE_IMAGE(free_image)
{
    Core->VkCore.DestroyImageSampler((*Image)->Sampler);
    Core->VkCore.DestroyImageView((*Image)->View);
    Core->VkCore.DestroyVmaImage((*Image)->Handle, (*Image)->Memory);
    
    memory_release(Core->Memory, (*Image));
    (*Image) = NULL;
}

COPY_BUFFER_TO_IMAGE(copy_buffer_to_image)
{
    // TODO(Dustin): Find an alternative to idling.
    Core->VkCore.Idle();
    
    // Transition the image to TransferDestinationOptimal 
    if (Image->CurrentLayout == ImageLayout_Undefined)
    {
        Core->VkCore.TransitionImageLayout(Core->Renderer->CommandPool,
                                           Image->Handle,
                                           Image->Format,
                                           VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           Image->MipLevels);
    }
    else if (Image->CurrentLayout == ImageLayout_TransferDst)
    {
        // Don't have to do anything. Ready to recieve the data
    }
    else if (Image->CurrentLayout == ImageLayout_ShaderReadOnly)
    {
        Core->VkCore.TransitionImageLayout(Core->Renderer->CommandPool,
                                           Image->Handle,
                                           Image->Format,
                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           Image->MipLevels);
    }
    
    // Copy the buffer into the image
    Core->VkCore.CopyBufferToImage(Core->Renderer->CommandPool,
                                   UploadBuffer->Handle,
                                   Image->Handle,
                                   Image->Width, Image->Height);
    
    // Transition the image to ShaderReadOnlyOptimal
    Core->VkCore.TransitionImageLayout(Core->Renderer->CommandPool,
                                       Image->Handle,
                                       Image->Format,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                       Image->MipLevels);
    
    Image->CurrentLayout = ImageLayout_ShaderReadOnly;
}

GET_IMAGE_DIMENSIONS(get_image_dimensions)
{
    *Width  = Image->Width;
    *Height = Image->Height;
}

CREATE_DESCRIPTOR_SET_LAYOUT(create_descriptor_set_layout)
{
    descriptor_layout Result = (descriptor_layout)memory_alloc(Core->Memory, sizeof(mp_descriptor_layout));
    
    Result->Handle = Core->VkCore.CreateDescriptorSetLayout(LayoutInfo->Bindings, 
                                                            LayoutInfo->BindingsCount);
    
    *Layout = Result;
}

FREE_DESCRIPTOR_SET_LAYOUT(free_descriptor_set_layout)
{
    Core->VkCore.DestroyDescriptorSetLayout((*Layout)->Handle);
    memory_release(Core->Memory, (*Layout));
    (*Layout) = NULL;
}

CREATE_DESCRIPTOR_SET(create_descriptor_set) 
{
    descriptor_set Result = (descriptor_set)memory_alloc(Core->Memory, sizeof(mp_descriptor_set));
    
    u32 SwapChainImageCount = Core->VkCore.GetSwapChainImageCount();
    Result->HandleCount = SwapChainImageCount;
    
    VkDescriptorSetLayout *Layouts = (VkDescriptorSetLayout*)memory_alloc(Core->Memory, 
                                                                          sizeof(VkDescriptorSetLayout) * SwapChainImageCount);
    for (u32 LayoutIdx = 0; LayoutIdx < SwapChainImageCount; ++LayoutIdx)
        Layouts[LayoutIdx] = SetInfo->Layout->Handle;
    
    VkDescriptorSetAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    AllocInfo.descriptorPool     = Core->Renderer->DescriptorPool;
    AllocInfo.descriptorSetCount = SwapChainImageCount;
    AllocInfo.pSetLayouts        = Layouts;
    
    Result->Handles = (VkDescriptorSet*)memory_alloc(Core->Memory, 
                                                     sizeof(VkDescriptorSet) * SwapChainImageCount);
    
    Core->VkCore.CreateDescriptorSets(Result->Handles,
                                      AllocInfo);
    
    memory_release(Core->Memory, Layouts);
    
    Result->Binding = SetInfo->Binding;
    Result->Set     = SetInfo->Set;
    
    *Set = Result;
}

FREE_DESCRIPTOR_SET(free_descriptor_set) 
{
    memory_release(Core->Memory, (*Set)->Handles);
    memory_release(Core->Memory, (*Set));
    (*Set) = NULL;
}

BIND_BUFFER_TO_DESCRIPTOR_SET(bind_buffer_to_descriptor_set)
{
    for (u32 i = 0; i < Set->HandleCount; ++i) 
    {
        VkWriteDescriptorSet DescriptorWrites[1] = {};
        
#if 0
        VkDescriptorBufferInfo FrameBufferInfo = {};
        FrameBufferInfo.buffer                 = ObjectDataBuffer->Buffer.Handles[i].Handle;
        FrameBufferInfo.offset                 = WriteInfo->Offset;
        FrameBufferInfo.range                  = WriteInfo->Range;
#else
        VkDescriptorImageInfo ImageInfo = {};
        ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ImageInfo.imageView   = WriteInfo->Image->View;
        ImageInfo.sampler     = WriteInfo->Image->Sampler;
#endif
        
        DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[0].dstSet           = Set->Handles[i];
        DescriptorWrites[0].dstBinding       = Set->Binding;
        DescriptorWrites[0].dstArrayElement  = 0;
        DescriptorWrites[0].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        DescriptorWrites[0].descriptorCount  = 1;
        DescriptorWrites[0].pBufferInfo      = NULL;
        DescriptorWrites[0].pImageInfo       = &ImageInfo;
        DescriptorWrites[0].pTexelBufferView = NULL;
        
        Core->VkCore.UpdateDescriptorSets(DescriptorWrites, 1);
    }
}

//~ Command List commands

CMD_BIND_PIPELINE(cmd_bind_pipeline)
{
    cmd_bind_pipeline_info Info = {0};
    Info.Pipeline = Pipeline;
    
    mp_command_list_add(CommandList, CmdType_BindPipeline, sizeof(cmd_bind_pipeline_info), &Pipeline);
}

CMD_DRAW(cmd_draw)
{
    mp_command_list_add(CommandList, CmdType_Draw, sizeof(mp_render_component), RenderComponent);
}

CMD_SET_OBJECT_WORLD_DATA(cmd_set_object_world_data)
{
    cmd_set_object_world_data_info Data = {0};
    Data.Position = Position;
    Data.Scale    = Scale;
    Data.Rotation = Rotation;
    
    mp_command_list_add(CommandList, CmdType_UpdateObjectData, 
                        sizeof(cmd_set_object_world_data_info), 
                        &Data);
}

CMD_SET_CAMERA(cmd_set_camera)
{
    camera_data Data = {0};
    Data.Projection = Projection;
    Data.View = View;
    
    mp_command_list_add(CommandList, CmdType_SetCamera, 
                        sizeof(camera_data), 
                        &Data);
}

CMD_BIND_DESCRIPTOR_SET(cmd_bind_descriptor_set)
{
    mp_command_list_add(CommandList, CmdType_BindDescriptor, 
                        sizeof(mp_descriptor_set), 
                        Set);
}

SET_RENDER_MODE(set_render_mode)
{
    if (Mode & RenderMode_NormalVis)
    {
        // RenderMode is currently NormalVis, unset it
        if (Core->Renderer->RenderMode & Mode)
        {
            Core->Renderer->RenderMode &= (u32)~Mode;
        }
        else
        {
            Core->Renderer->RenderMode |= (u32)Mode;
        }
    }
    else
    {
        Core->Renderer->RenderMode = (u32)Mode;
    }
}

GET_RENDER_MODE(get_render_mode)
{
    return Core->Renderer->RenderMode;
}

#undef EXTERN_GRAPHICS_API