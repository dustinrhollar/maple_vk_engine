
typedef struct terrain_vertex
{
    vec3 Position;
    vec3 Normal;
    vec2 Uv;
    vec3 Color;
} terrain_vertex;

typedef struct terrain_color
{
    vec3 Color;
    r32  Height; // Start Height
} terrain_color;
#define TERRAIN_COLOR_COUNT 7
#define norm_color(v) (r32)(v) / 255.0f

typedef struct terrain_settings 
{
    //~ Mesh Information
    
    u32 TerrainWidth;
    u32 TerrainHeight;
    
    vec3       Scale;
    vec3       Position;
    quaternion Rotation;
    
    //~ Map dimensions
    
    u32 HeightmapWidth;
    u32 HeightmapHeight;
    r32 HeightmapScale;
    
    //~ Simplex Information
    
    i64 Seed;
    i32 NumOctaves;
    // Control the roughness of the output
    // should be 0 ... 1
    r32 Persistence;    
    // Control the width of "gaps" on the map
    // should be >= 1
    r32 Lacunarity;     
    // Control the "zoom" for the noise
    // should be > 0
    r32 SimplexScale;     
    
    //~ Stored Heightmaps (so that memory doesn't have to be reallocated if not needed)
    
    heightmap SimplexHeightmap;
    heightmap FinalHeightmap;
    
    //~ Define how the terrain is shaded
    
    terrain_color Colors[TERRAIN_COLOR_COUNT];
    
} terrain_settings;

file_internal void terrain_generate_base_mesh(terrain *Terrain);
file_internal void terrain_generate_heightmap(terrain *Terrain);
file_internal void terrain_write(terrain *Terrain);

file_global terrain_settings Settings = 
{
    // Mesh information
    .TerrainWidth  = 1024, 
    .TerrainHeight = 1024,
    .Scale         = { 250.0f, 50.0f, 250.0f },     
    .Position      = { 0.0f, 0.0f, 0.0f },     
    .Rotation      = { 0.0f, 0.0f, 0.0f, 1.0f },
    
    // Heightmap information
    .HeightmapWidth  = 1024,
    .HeightmapHeight = 1024,
    .HeightmapScale  = 1.0f,
    
    // Simplex Noise Simulation
    .Seed         = 152002,
    .NumOctaves   = 8,     // Control the number of octaves used,     x > 0
    .Persistence  = 0.3f,  // Control the roughness of the output,    0 <= x >= 1
    .Lacunarity   = 2.5f,  // Control the width of "gaps" on the map, x >= 1 
    .SimplexScale = 500.0f,  // Control the "zoom" for the noise,     x >= 0
    
    // Shading
    .Colors = {
        { .Color = { norm_color(0x00), norm_color(0x00), norm_color(0xFF) }, .Height = 0.00f }, // Water
        { .Color = { norm_color(0xFF), norm_color(0xFD), norm_color(0xD0) }, .Height = 0.10f }, // Beach
        { .Color = { norm_color(0x22), norm_color(0x8B), norm_color(0x22) }, .Height = 0.20f }, // Forest
        { .Color = { norm_color(0x0B), norm_color(0x2F), norm_color(0x25) }, .Height = 0.30f }, // Jungle
        { .Color = { norm_color(0xF0), norm_color(0xe6), norm_color(0x8C) }, .Height = 0.50f }, // Savannah
        { .Color = { norm_color(0xED), norm_color(0xC9), norm_color(0xAF) }, .Height = 0.70f }, // Desert
        { .Color = { norm_color(0xFF), norm_color(0xFF), norm_color(0xFF) }, .Height = 0.90f }, // SNow
        
    }, 
};

file_internal void terrain_write(terrain *Terrain)
{
    terrain_vertex *Vertices = NULL;
    
    Graphics.map_upload_buffer(Terrain->VertexUploadBuffer, 0, (void**) &Vertices);
    file_t NormalFile = PlatformApi->open_file("normal_file_1.txt", false);
    
    PlatformApi->write_file(NormalFile, "%d %d\n", 
                            Settings.HeightmapWidth,
                            Settings.HeightmapHeight);
    
    for (u32 r = 0; r < Terrain->Height; ++r)
    {
        u32 base = r * Terrain->Width;
        for (u32 c = 0; c < Terrain->Width; ++c)
        {
            PlatformApi->write_file(NormalFile, "%f %f %f\n", 
                                    Vertices[base + c].Normal.x,
                                    Vertices[base + c].Normal.y,
                                    Vertices[base + c].Normal.z);
        }
    }
    
    PlatformApi->flush_file(NormalFile);
    PlatformApi->close_file(NormalFile);
    
    file_t HeightFile = PlatformApi->open_file("heightmap_blurred.ppm", false);
    PlatformApi->write_file(HeightFile, "P6\n%d %d\n255\n", 
                            Settings.HeightmapWidth,
                            Settings.HeightmapHeight);
    
    int length = Settings.HeightmapWidth * Settings.HeightmapHeight;
    for (int k = 0; k < length; ++k)
    {
        u8 n = Settings.FinalHeightmap.Ptr[k] * 255;
        
        PlatformApi->write_file(HeightFile, "%c%c%c", n, n, n);
    }
    
    PlatformApi->flush_file(HeightFile);
    PlatformApi->close_file(HeightFile);
    
    Graphics.unmap_upload_buffer(Terrain->VertexUploadBuffer, (void**) &Vertices);
}

// Place terrain generation code here
void terrain_update(terrain *Terrain)
{
    terrain_generate_heightmap(Terrain);
    terrain_generate_base_mesh(Terrain);
}

void terrain_draw(command_list     CommandList, 
                  terrain         *Terrain)
{
    if (Terrain->RenderInfo)
    {
        Graphics.cmd_bind_pipeline(Terrain->Pipeline, CommandList);
        
        Graphics.cmd_bind_descriptor_set(CommandList, Terrain->TerrainSet);
        
        Graphics.cmd_set_object_world_data(CommandList, Settings.Position, Settings.Scale, Settings.Rotation);
        Graphics.cmd_draw(Terrain->RenderInfo, CommandList);
        
        // Render the normal visualization
        u32 Mode = Graphics.get_render_mode();
        if (Mode & RenderMode_NormalVis)
        {
            Graphics.cmd_bind_pipeline(Terrain->NormalVis, CommandList);
            Graphics.cmd_bind_descriptor_set(CommandList, Terrain->TerrainSet);
            
            // TODO(Dustin): Currently have to set world dat twice, which is not good
            Graphics.cmd_set_object_world_data(CommandList, Settings.Position, Settings.Scale, Settings.Rotation);
            Graphics.cmd_draw(Terrain->RenderInfo, CommandList);
        }
    }
}

// NOTE(Dustin): Custom Add/Remove steps in generating the final heightmap
file_internal void terrain_generate_heightmap(terrain *Terrain)
{
    if (!Settings.SimplexHeightmap.Handle)
    {
        heightmap_init(&Settings.SimplexHeightmap, Settings.HeightmapWidth,  Settings.HeightmapWidth);
        heightmap_init(&Settings.FinalHeightmap, Settings.HeightmapWidth,  Settings.HeightmapWidth);
    }
    else
    {
        heightmap_free(&Settings.SimplexHeightmap);
        heightmap_init(&Settings.SimplexHeightmap, Settings.HeightmapWidth,  Settings.HeightmapWidth);
        
        heightmap_free(&Settings.FinalHeightmap);
        heightmap_init(&Settings.FinalHeightmap, Settings.HeightmapWidth,  Settings.HeightmapWidth);
    }
    
    simplex_noisemap_sim_info SimplexInfo = {0};
    SimplexInfo.Seed        = Settings.Seed;
    SimplexInfo.NumOctaves  = Settings.NumOctaves;
    SimplexInfo.Persistence = Settings.Persistence;
    SimplexInfo.Lacunarity  = Settings.Lacunarity;  
    SimplexInfo.Scale       = Settings.SimplexScale;      
    //simplex_simulate_noisemap(&Settings.SimplexHeightmap, &SimplexInfo);
    simplex_simulate_noisemap(&Settings.FinalHeightmap, &SimplexInfo);
    
    //gaussian_blur(&Settings.SimplexHeightmap, &Settings.FinalHeightmap, 1);
    
    if (!Terrain->Heightmap)
    {
        image_create_info ImageInfo = {0};
        ImageInfo.Width        = Settings.HeightmapWidth;
        ImageInfo.Height       = Settings.HeightmapHeight;
        ImageInfo.MipLevels    = 1;
        ImageInfo.ImageFormat  = VK_FORMAT_R32_SFLOAT;
        ImageInfo.MagFilter    = VK_FILTER_NEAREST;
        ImageInfo.MinFilter    = VK_FILTER_NEAREST;
        ImageInfo.AddressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        ImageInfo.AddressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        ImageInfo.AddressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        
        Graphics.create_image(&Terrain->Heightmap, &ImageInfo);
        
        descriptor_write_info WriteInfo = {0};
        WriteInfo.Image  = Terrain->Heightmap;
        WriteInfo.Offset = 0;
        WriteInfo.Range  = VK_WHOLE_SIZE;
        
        Graphics.bind_buffer_to_descriptor_set(Terrain->TerrainSet, &WriteInfo);
    }
    else
    {
        u32 OldWidth, OldHeight;
        Graphics.get_image_dimensions(Terrain->Heightmap, &OldWidth, &OldHeight);
        
        if (OldWidth != Settings.HeightmapWidth || OldHeight != Settings.HeightmapHeight)
        {
            Graphics.resize_image(Terrain->Heightmap, Settings.HeightmapWidth, Settings.HeightmapHeight);
            
            descriptor_write_info WriteInfo = {0};
            WriteInfo.Image  = Terrain->Heightmap;
            WriteInfo.Offset = 0;
            WriteInfo.Range  = VK_WHOLE_SIZE;
            
            Graphics.bind_buffer_to_descriptor_set(Terrain->TerrainSet, &WriteInfo);
        }
    }
    
    Graphics.copy_buffer_to_image(Terrain->Heightmap, Settings.FinalHeightmap.Handle);
}


file_internal void terrain_generate_base_mesh(terrain *Terrain)
{
    r32 MinX = -0.5f;
    r32 MaxX =  0.5f;
    
    r32 MinZ = -0.5f;
    r32 MaxZ =  0.5f;
    
    r32 StepX = (r32)(MaxX - MinX) / (r32)Settings.TerrainWidth;
    r32 StepZ = (r32)(MaxZ - MinZ) / (r32)Settings.TerrainHeight;
    
    Terrain->Width       = Settings.TerrainWidth;
    Terrain->Height      = Settings.TerrainHeight;
    
    u32 VertexCount = (Terrain->Width) * (Terrain->Height);
    u32 IndexCount  = (Terrain->Width * Terrain->Height) + (Terrain->Width - 1) * (Terrain->Width - 2);
    
    Graphics.resize_upload_buffer(Terrain->VertexUploadBuffer, VertexCount * sizeof(terrain_vertex));
    Graphics.resize_upload_buffer(Terrain->IndexUploadBuffer, IndexCount * sizeof(u32));
    
    terrain_vertex *Vertices = NULL;
    u32 *Indices = NULL;
    
    Graphics.map_upload_buffer(Terrain->VertexUploadBuffer, 0, (void**) &Vertices);
    Graphics.map_upload_buffer(Terrain->IndexUploadBuffer,  0, (void**) &Indices);
    
    // Generate the grid information
    // Create the vertex list
    u32 Idx = 0;
    for (u32 r = 0; r < Terrain->Height; ++r)
    {
        u32 base = r * Terrain->Width;
        for (u32 c = 0; c < Terrain->Width; ++c)
        {
            //~ Calculate the UVs
            
            r32 uvx = Settings.HeightmapScale * (c / (r32)Settings.HeightmapWidth);
            r32 uvz = Settings.HeightmapScale * (r / (r32)Settings.HeightmapHeight);
            
            uvx = uvx - (i32)uvx;
            uvz = uvz - (i32)uvz;
            
            Vertices[base + c].Uv.x = uvx;
            Vertices[base + c].Uv.y = uvz;
            
            //~ Calculate the Position
            
            // Sample the heightmap
            r32 Height = heightmap_sample(&Settings.FinalHeightmap, Vertices[base + c].Uv);
            assert(Height <= 1.0f && "Height is greater than 1.0f!");
            
            vec3 Position = { 
                MinX + (r32)c * StepX, 
                Height,
                MinZ + (r32)r * StepZ 
            };
            
            Vertices[base + c].Position = Position;
            
            //~ Set the colors for the terrain
            
            vec3 Color = { 0.5f, 0.5f, 0.5f };
            
            for (u32 i = 0; i < TERRAIN_COLOR_COUNT; ++i)
            {
                if (i == TERRAIN_COLOR_COUNT - 1)
                {
                    // Upper bound -> 0.8 and 1.0f
                    Color  = Settings.Colors[i].Color;
                }
                else if (Height >= Settings.Colors[i].Height && 
                         Height < Settings.Colors[i + 1].Height)
                {
                    
#if 1
                    r32 t = inv_lerp(Settings.Colors[i].Height, Settings.Colors[i + 1].Height, Height);
                    Color.x = lerp(Settings.Colors[i].Color.x, Settings.Colors[i + 1].Color.x, t);
                    Color.y = lerp(Settings.Colors[i].Color.y, Settings.Colors[i + 1].Color.y, t);
                    Color.z = lerp(Settings.Colors[i].Color.z, Settings.Colors[i + 1].Color.z, t);
#else
                    Color.x = 0.5f;
                    Color.y = 0.5f;
                    Color.z = 0.5f;
#endif
                    break;
                }
            }
            
            Vertices[base + c].Color = Color;
        }
    }
    
    for (u32 r = 0; r < Terrain->Height; ++r)
    {
        u32 base = r * Terrain->Width;
        for (u32 c = 0; c < Terrain->Width; ++c)
        {
            
            //~ Calculate the Normal
            
            vec3 Normal = {0.0f, 0.0f, 0.0f};
            
            if (r > 0 && r < Terrain->Height - 1 && c > 0 && c < Terrain->Width - 1)
            {
                vec3 Pos = Vertices[base + c].Position;
                
                u32 idx1 = ((r + 0) * Terrain->Width) + (c - 1);
                u32 idx2 = ((r + 1) * Terrain->Width) + (c + 0);
                u32 idx3 = ((r + 0) * Terrain->Width) + (c + 1);
                u32 idx4 = ((r - 1) * Terrain->Width) + (c + 0);
                
                vec3 Pos1 = { 
                    MinX + (r32)(c - 1) * StepX, 
                    //Settings.HeightScale * heightmap_sample(&Settings.SimplexHeightmap, Vertices[idx1].Uv), 
                    Vertices[idx1].Position.y,
                    MinZ + (r32)(r + 0) * StepZ 
                };
                
                vec3 Pos2 = { 
                    MinX + (r32)(c + 0) * StepX, 
                    //Settings.HeightScale * heightmap_sample(&Settings.SimplexHeightmap, Vertices[idx2].Uv), 
                    Vertices[idx2].Position.y,
                    MinZ + (r32)(r + 1) * StepZ 
                };
                
                vec3 Pos3 = { 
                    MinX + (r32)(c + 1) * StepX, 
                    //Settings.HeightScale * heightmap_sample(&Settings.SimplexHeightmap, Vertices[idx3].Uv), 
                    Vertices[idx3].Position.y,
                    MinZ + (r32)(r + 0) * StepZ 
                };
                
                vec3 Pos4 = { 
                    MinX + (r32)(c + 0) * StepX, 
                    //Settings.HeightScale * heightmap_sample(&Settings.SimplexHeightmap, Vertices[idx4].Uv), 
                    Vertices[idx4].Position.y,
                    MinZ + (r32)(r - 1) * StepZ 
                };
                
                vec3 v1 = vec3_sub(Pos1, Pos);
                vec3 v2 = vec3_sub(Pos2, Pos);
                vec3 v3 = vec3_sub(Pos3, Pos);
                vec3 v4 = vec3_sub(Pos4, Pos);
                
                vec3 v12 = vec3_cross(v1, v2);
                vec3 v23 = vec3_cross(v2, v3);
                vec3 v34 = vec3_cross(v3, v4);
                vec3 v41 = vec3_cross(v4, v1);
                
                v12 = vec3_norm(v12);
                v23 = vec3_norm(v23);
                v34 = vec3_norm(v34);
                v41 = vec3_norm(v41);
                
                Normal = vec3_add(Normal, v12);
                Normal = vec3_add(Normal, v23);
                Normal = vec3_add(Normal, v34);
                Normal = vec3_add(Normal, v41);
                
                Normal = vec3_norm(Normal);
            }
            else
            {
                Normal.y = 1.0f;
            }
            
            Vertices[base + c].Normal   = Normal;
            //Vertices[base + c].Color = Vertices[base + c].Normal; 
        }
    }
    
    // Create the indices list
    u32 i = 0;
    for (u32 r = 0; r < Terrain->Height - 1; r++)
    {
        if ((r & 1) == 0)
        { // even rows
            for (u32 c = 0; c < Terrain->Width; c++)
            {
                Indices[i++] = (r + 0) * Terrain->Width + c;
                Indices[i++] = (r + 1) * Terrain->Width + c;
            }
        }
        else
        {
            for (u32 c = Terrain->Width-1; c > 0; c--)
            {
                Indices[i++] = c + (r + 1) * Terrain->Width;
                Indices[i++] = c - 1 + (r + 0) * Terrain->Width;
            }
        }
        
        if ((Terrain->Height & 1) && Terrain->Height > 2)
        {
            Indices[i++] = (Terrain->Height - 1) * Terrain->Width;
        }
    }
    
    // Copy the new data into the render component
    
    if (Terrain->RenderInfo)
    {
        Graphics.copy_upload_buffer(Terrain->VertexUploadBuffer, Terrain->RenderInfo);
        Graphics.copy_upload_buffer(Terrain->IndexUploadBuffer, Terrain->RenderInfo);
        Graphics.set_render_component_info(Terrain->RenderInfo, true, VK_INDEX_TYPE_UINT32, IndexCount);
    }
    else 
    {
        render_component_create_info RenderInfo = {0};
        RenderInfo.VertexData   = Vertices;
        RenderInfo.VertexCount  = VertexCount;
        RenderInfo.VertexStride = sizeof(terrain_vertex);
        RenderInfo.HasIndices   = true;
        RenderInfo.IndexData    = Indices;
        RenderInfo.IndexCount   = IndexCount;
        RenderInfo.IndexStride  = sizeof(u32);
        Graphics.create_render_component(&RenderInfo, &Terrain->RenderInfo);
    }
    
    Graphics.unmap_upload_buffer(Terrain->VertexUploadBuffer, (void**) &Vertices);
    Graphics.unmap_upload_buffer(Terrain->IndexUploadBuffer,  (void**) &Indices);
}

// Handle Settings that grabs graphics resource that have to be freed. 
void terrain_shutdown()
{
    heightmap_free(&Settings.SimplexHeightmap);
    heightmap_free(&Settings.FinalHeightmap);
}