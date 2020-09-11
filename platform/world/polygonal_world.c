
typedef struct voxel_vertex
{
    vec3 Position;
    vec3 Normal;
    vec2 Uv;
    vec3 Color;
} voxel_vertex;

file_global VkVertexInputAttributeDescription VoxelVertexAttributes[4] = {
    // Location, Binding, Format, Offset
    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(voxel_vertex, Position) }, // Position
    { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(voxel_vertex, Normal)   }, // Normal
    { 2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(voxel_vertex, Uv)       }, // UVs
    { 3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(voxel_vertex, Color)    }, // Color
};

file_global VkVertexInputBindingDescription VoxelVertexBindingDescription = {
    // Binding, Stride, InputRate
    0, sizeof(voxel_vertex), VK_VERTEX_INPUT_RATE_VERTEX
};

file_global VkVertexInputAttributeDescription VoxelNormalVertexAttributes[3] = {
    // Location, Binding, Format, Offset
    { 0, 0, VK_FORMAT_R32G32_SFLOAT,    0                           }, // Position
    { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(vec2)                }, // Normal
};

file_global VkVertexInputBindingDescription VoxelNormalVertexBindingDescription = {
    // Binding, Stride, InputRate
    0, sizeof(voxel_vertex), VK_VERTEX_INPUT_RATE_VERTEX
};

#if 0
file_internal VkVertexInputBindingDescription voxel_vertex_binding_description()
{
    VkVertexInputBindingDescription Result = {};
    
    Result.binding   = 0;
    Result.stride    = sizeof(voxel_vertex);
    Result.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    return Result;
}
#endif

void world_init(world *World)
{
    command_pool_create_info PoolInfo = {};
    PoolInfo.CommandPool = &World->CommandPool;
    Graphics.create_command_pool(&PoolInfo);
    
    command_list_create_info ListInfo = {};
    ListInfo.CommandList = &World->CommandList;
    ListInfo.CommandPool = World->CommandPool;
    Graphics.create_command_list(&ListInfo);
    
    //~ Create the descriptors for the heightmap
    {
        u32 HeightmapBinding = 0;
        u32 HeightmapSet     = 2;
        
        VkDescriptorSetLayoutBinding Bindings[1]= {};
        Bindings[0].binding            = HeightmapBinding;
        Bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        Bindings[0].descriptorCount    = 1;
        Bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
        
        descriptor_layout_create_info LayoutInfo = {0};
        LayoutInfo.Bindings      = Bindings;
        LayoutInfo.BindingsCount = 1;
        
        Graphics.create_descriptor_set_layout(&World->Terrain.TerrainLayout, &LayoutInfo);
        
        descriptor_set_create_info SetInfo = {0};
        SetInfo.Layout  = World->Terrain.TerrainLayout;
        SetInfo.Set     = HeightmapSet;
        SetInfo.Binding = HeightmapBinding;
        
        Graphics.create_descriptor_set(&World->Terrain.TerrainSet, &SetInfo);
    }
    
    
    //~ Initialize the Terrain Pipeline
    // NOTE(Dustin): This should probably occur in a voxel_init function instead...
    {
        pipeline_create_info PipelineCreateInfo = {0};
        PipelineCreateInfo.VertexShader      = "data/shaders/simple_tri.vert.spv";
        PipelineCreateInfo.FragmentShader    = "data/shaders/simple_tri.frag.spv";
        PipelineCreateInfo.GeometryShader    = NULL;
        PipelineCreateInfo.TessControlShader = NULL;
        PipelineCreateInfo.TessEvalShader    = NULL;
        
        //PipelineCreateInfo.VertexInputInfo = {0};
        PipelineCreateInfo.VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        PipelineCreateInfo.VertexInputInfo.pNext                           = NULL;
        PipelineCreateInfo.VertexInputInfo.flags                           = 0;
        PipelineCreateInfo.VertexInputInfo.vertexBindingDescriptionCount   = 1;
        PipelineCreateInfo.VertexInputInfo.pVertexBindingDescriptions      = &VoxelVertexBindingDescription;
        PipelineCreateInfo.VertexInputInfo.vertexAttributeDescriptionCount 
            = sizeof(VoxelVertexAttributes) / sizeof(VoxelVertexAttributes[0]);
        PipelineCreateInfo.VertexInputInfo.pVertexAttributeDescriptions    = VoxelVertexAttributes;
        // NOTE(Dustin): Set as dynamic state 
        PipelineCreateInfo.Viewport               = NULL;
        PipelineCreateInfo.Scissor                = NULL;
        PipelineCreateInfo.ScissorCount           = 1;
        PipelineCreateInfo.ViewportCount          = 1;
        // Input assembly
        PipelineCreateInfo.Topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        // rasterizer
        PipelineCreateInfo.PolygonMode            = VK_POLYGON_MODE_FILL;
        PipelineCreateInfo.FrontFace              = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        // Multisample
        PipelineCreateInfo.HasMultisampling       = false;
        PipelineCreateInfo.MuliSampleSamples      = VK_SAMPLE_COUNT_1_BIT;
        // Depth/Stencil
        PipelineCreateInfo.HasDepthStencil        = true;
        // LayoutCreateInfo
        PipelineCreateInfo.PushConstants          = NULL;
        PipelineCreateInfo.PushConstantsCount     = 0;
        // Descriptors
        PipelineCreateInfo.DescriptorLayouts      = &World->Terrain.TerrainLayout;
        PipelineCreateInfo.DescriptorLayoutsCount = 1;
        // Render Pass
        //PipelineCreateInfo.RenderPass = Core->Renderer->RenderPass;
        
        Graphics.create_pipeline(&PipelineCreateInfo, &World->Terrain.Pipeline);
        
        //~ Initialize Normal Vis for Terrain
        
        //PipelineCreateInfo = {0};
        PipelineCreateInfo.VertexShader      = "data/shaders/terrain_normal_vis.vert.spv";
        PipelineCreateInfo.FragmentShader    = "data/shaders/terrain_normal_vis.frag.spv";
        PipelineCreateInfo.GeometryShader    = "data/shaders/terrain_normal_vis.geom.spv";
        PipelineCreateInfo.TessControlShader = NULL;
        PipelineCreateInfo.TessEvalShader    = NULL;
        
        Graphics.create_pipeline(&PipelineCreateInfo, &World->Terrain.NormalVis);
    }
    
    // initalize the render component
    {
        World->Terrain.RenderInfo = NULL;
        World->Terrain.Heightmap  = NULL;
    }
    
    // Initialize the upload buffer
    Graphics.create_upload_buffer(&World->Terrain.VertexUploadBuffer, UploadBuffer_Vertex, 0);
    Graphics.create_upload_buffer(&World->Terrain.IndexUploadBuffer, UploadBuffer_Index, 0);
    
    vec3 Scale = { 1.0f, 1.0f, 1.0f };
    vec3 Position = { 0.0f, 0.0f, 0.0f };
    quaternion Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    
    World->Terrain.Scale    = Scale;
    World->Terrain.Position = Position;
    World->Terrain.Rotation = Rotation;
    
}


void world_free(world *World)
{
    Graphics.free_command_list(&World->CommandList);
    Graphics.free_command_pool(&World->CommandPool);
    Graphics.free_pipeline(&World->Terrain.Pipeline);
    Graphics.free_pipeline(&World->Terrain.NormalVis);
    Graphics.free_descriptor_set_layout(&World->Terrain.TerrainLayout);
    Graphics.free_descriptor_set(&World->Terrain.TerrainSet);
    if (World->Terrain.RenderInfo) Graphics.free_render_component(&World->Terrain.RenderInfo);
    if (World->Terrain.Heightmap) Graphics.free_image(&World->Terrain.Heightmap);
    Graphics.free_upload_buffer(&World->Terrain.VertexUploadBuffer);
    Graphics.free_upload_buffer(&World->Terrain.IndexUploadBuffer);
}

#if 0
void world_draw(world *World)
{
    Graphics.cmd_set_camera(World->CommandList, mat4_diag(1.0f), mat4_diag(1.0f));
    
    Graphics.cmd_bind_pipeline(World->TestTriangle.Pipeline, World->CommandList);
    
    vec3 Position = {0,0,0};
    vec3 Scale = {1,1,1};
    
    // For zero rotation, a quaternion should have w component as 1
    quaternion Rotation = {0,0,0,1};
    Graphics.cmd_set_object_world_data(World->CommandList, Position, Scale, Rotation);
    
    Graphics.cmd_draw(World->TestTriangle.RenderInfo, World->CommandList);
    Graphics.execute_command_list(World->CommandList);
}
#endif