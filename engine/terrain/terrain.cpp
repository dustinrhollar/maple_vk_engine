
file_internal void CreateHeightmap(asset_terrain *Terrain, terrain_settings *Settings);


void CreateTerrain(asset_registry *AssetRegistry, terrain_settings *Settings)
{
    asset_terrain_create_info CreateInfo = {};
    asset_id TerrainId = CreateAsset(AssetRegistry, Asset_Terrain, &CreateInfo);
    AssetRegistry->TerrainId = TerrainId;
    
    asset_terrain *Terrain = &(GetAsset(TerrainId)->Terrain);
    
    // Create the mvp buffer
    buffer_create_info MvpBufferInfo  = {};
    MvpBufferInfo.Size                = sizeof(mat4) * 3;
    MvpBufferInfo.Usage               = BufferUsage_Default;
    MvpBufferInfo.CpuAccessFlags      = BufferCpuAccess_None;
    MvpBufferInfo.BindFlags           = BufferBind_ConstantBuffer;
    MvpBufferInfo.MiscFlags           = BufferMisc_None;
    MvpBufferInfo.StructureByteStride = 0;
    Terrain->MvpBuffer = CreateResource(GlobalResourceRegistry, Resource_Buffer, &MvpBufferInfo);
    
    CreateTerrainMaterial(Terrain);
    GenerateTerrainGrid(Terrain, Settings->TerrainWidth, Settings->TerrainHeight);
    CreateTerrainVertexBuffers(Terrain);
    CreateHeightmap(Terrain, Settings);
}

void ResetTerrain(asset_terrain *Terrain, terrain_settings *Settings)
{
    if (Settings->TerrainMeshUpdated)
    {
        if (Terrain->Vertices) pfree(&GlobalMemory, Terrain->Vertices);
        if (Terrain->Indices)  pfree(&GlobalMemory, Terrain->Indices);
        
        // TODO(Dustin): Destroy buffer resources
        if (!IsValidResource(Terrain->VertexBuffer)) FreeResource(Terrain->VertexBuffer);
        if (!IsValidResource(Terrain->IndexBuffer))  FreeResource(Terrain->IndexBuffer);
        
        GenerateTerrainGrid(Terrain, Settings->TerrainWidth, Settings->TerrainHeight);
        CreateTerrainVertexBuffers(Terrain);
    }
    
    if (Settings->HeightmapUpdated)
    {
        if (Terrain->Heightmap) pfree(&GlobalMemory, Terrain->Heightmap);
        if (Terrain->Colormap)  pfree(&GlobalMemory, Terrain->Colormap);
        if (Terrain->Normalmap) pfree(&GlobalMemory, Terrain->Normalmap);
        
        // Destroy texture resources
        if (!IsValidResource(Terrain->HeightmapTexture))  FreeResource(Terrain->HeightmapTexture);
        if (!IsValidResource(Terrain->ColormapTexture))   FreeResource(Terrain->ColormapTexture);
        if (!IsValidResource(Terrain->NormalmapTexture))  FreeResource(Terrain->NormalmapTexture);
        
        CreateHeightmap(Terrain, Settings);
    }
}

void DestroyTerrain(asset_terrain *Terrain)
{
    if (Terrain->Vertices)  pfree(&GlobalMemory, Terrain->Vertices);
    if (Terrain->Indices)   pfree(&GlobalMemory, Terrain->Indices);
    if (Terrain->Heightmap) pfree(&GlobalMemory, Terrain->Heightmap);
    if (Terrain->Colormap)  pfree(&GlobalMemory, Terrain->Colormap);
    if (Terrain->Normalmap) pfree(&GlobalMemory, Terrain->Normalmap);
    
    Terrain->Width           = 0;
    Terrain->Height          = 0;
    Terrain->HeightmapWidth  = 0;
    Terrain->HeightmapHeight = 0;
    Terrain->VertexCount     = 0;
    Terrain->IndexCount      = 0;
    
    // Destroy texture resources
    if (!IsValidResource(Terrain->HeightmapTexture))  FreeResource(Terrain->HeightmapTexture);
    if (!IsValidResource(Terrain->ColormapTexture))   FreeResource(Terrain->ColormapTexture);
    if (!IsValidResource(Terrain->NormalmapTexture))  FreeResource(Terrain->NormalmapTexture);
}

// SOURCE: https://stackoverflow.com/questions/5915753/generate-a-plane-with-triangle-strips
void GenerateTerrainGrid(asset_terrain *Terrain, u32 width, u32 height)
{
    Terrain->Width       = width;
    Terrain->Height      = height;
    
    Terrain->VertexCount = (width) * (height);
    Terrain->IndexCount  = (width * height) + (width - 1) * (width - 2);
    
    Terrain->Vertices = palloc<terrain_vertex>(&GlobalMemory, Terrain->VertexCount);
    Terrain->Indices  = palloc<u32>(&GlobalMemory, Terrain->IndexCount);
    
    // Generate the grid information
    // Create the vertex list
    u64 idx = 0;
    for (u32 r = 0; r < height; ++r)
    {
        u32 base = r * width;
        for (u32 c = 0; c < width; ++c, idx += 6)
        {
            Terrain->Vertices[base + c].Position = {(r32)c, (r32)r};
            Terrain->Vertices[base + c].Normal   = {0.0f, 0.0f, 0.0f}; // TODO(Dustin)
            Terrain->Vertices[base + c].Uvs      = {c / (r32)width, r / (r32)height};
        }
    }
    
    // Create the indices list
    u32 i = 0;
    for (u32 r = 0; r < height - 1; r++)
    {
        if ((r & 1) == 0)
        { // even rows
            for (u32 c = 0; c < width; c++)
            {
                Terrain->Indices[i++] = (r + 0) * width + c;
                Terrain->Indices[i++] = (r + 1) * width + c;
            }
        }
        else
        {
            for (u32 c = width-1; c > 0; c--)
            {
                Terrain->Indices[i++] = c + (r + 1) * width;
                Terrain->Indices[i++] = c - 1 + (r + 0) * width;
            }
        }
        
        if ((height & 1) && height > 2)
        {
            Terrain->Indices[i++] = (height - 1) * width;
        }
    }
}

void CreateTerrainVertexBuffers(asset_terrain *Terrain)
{
#if 1
    
    buffer_create_info VertexBufferInfo  = {};
    VertexBufferInfo.Size                = Terrain->VertexCount * sizeof(terrain_vertex);
    VertexBufferInfo.Usage               = BufferUsage_Default;
    VertexBufferInfo.CpuAccessFlags      = BufferCpuAccess_None;
    VertexBufferInfo.BindFlags           = BufferBind_VertexBuffer;
    VertexBufferInfo.MiscFlags           = BufferMisc_None;
    VertexBufferInfo.StructureByteStride = sizeof(terrain_vertex);
    
    // Optional data
    VertexBufferInfo.Data             = Terrain->Vertices;
    VertexBufferInfo.SysMemPitch      = VertexBufferInfo.Size;
    VertexBufferInfo.SysMemSlicePitch = 0;
    
    Terrain->VertexBuffer = CreateResource(GlobalResourceRegistry, Resource_Buffer, &VertexBufferInfo);
    
    buffer_create_info IndexBufferInfo  = {};
    IndexBufferInfo.Size                = Terrain->IndexCount * sizeof(u32);
    IndexBufferInfo.Usage               = BufferUsage_Default;
    IndexBufferInfo.CpuAccessFlags      = BufferCpuAccess_None;
    IndexBufferInfo.BindFlags           = BufferBind_IndexBuffer;
    IndexBufferInfo.MiscFlags           = BufferMisc_None;
    IndexBufferInfo.StructureByteStride = sizeof(u32);
    
    // Optional data
    IndexBufferInfo.Data             = Terrain->Indices;
    IndexBufferInfo.SysMemPitch      = IndexBufferInfo.Size;
    IndexBufferInfo.SysMemSlicePitch = 0;
    
    Terrain->IndexBuffer = CreateResource(GlobalResourceRegistry, Resource_Buffer, &IndexBufferInfo);
    
#else
    
    terrain_vertex Temp[3] = {
        { {  0.0f,  0.5f }, {0, 0, 0}, {0, 0} },
        { {  0.5f, -0.5f }, {0, 0, 0}, {0, 0} },
        { { -0.5f, -0.5f }, {0, 0, 0}, {0, 0} },
    };
    
    u32 Indices[3] = { 0, 1, 2 };
    
    Terrain->VertexCount = 3;
    Terrain->IndexCount = 3;
    
    buffer_create_info VertexBufferInfo  = {};
    VertexBufferInfo.Size                = Terrain->VertexCount * sizeof(terrain_vertex);
    VertexBufferInfo.Usage               = BufferUsage_Default;
    VertexBufferInfo.CpuAccessFlags      = BufferCpuAccess_None;
    VertexBufferInfo.BindFlags           = BufferBind_VertexBuffer;
    VertexBufferInfo.MiscFlags           = BufferMisc_None;
    VertexBufferInfo.StructureByteStride = sizeof(terrain_vertex);
    
    // Optional data
    //VertexBufferInfo.Data             = Terrain->Vertices;
    VertexBufferInfo.Data             = Temp;
    VertexBufferInfo.SysMemPitch      = VertexBufferInfo.Size;
    VertexBufferInfo.SysMemSlicePitch = 0;
    
    Terrain->VertexBuffer = CreateResource(GlobalResourceRegistry, Resource_Buffer, &VertexBufferInfo);
    
    buffer_create_info IndexBufferInfo  = {};
    IndexBufferInfo.Size                = Terrain->IndexCount * sizeof(u32);
    IndexBufferInfo.Usage               = BufferUsage_Default;
    IndexBufferInfo.CpuAccessFlags      = BufferCpuAccess_None;
    IndexBufferInfo.BindFlags           = BufferBind_IndexBuffer;
    IndexBufferInfo.MiscFlags           = BufferMisc_None;
    IndexBufferInfo.StructureByteStride = sizeof(u32);
    
    // Optional data
    //IndexBufferInfo.Data             = Terrain->Indices;
    IndexBufferInfo.Data             = Indices;
    IndexBufferInfo.SysMemPitch      = IndexBufferInfo.Size;
    IndexBufferInfo.SysMemSlicePitch = 0;
    
    Terrain->IndexBuffer = CreateResource(GlobalResourceRegistry, Resource_Buffer, &IndexBufferInfo);
    
    
#endif
}

void CreateTerrainMaterial(asset_terrain *Terrain)
{
    file_t VertFile = PlatformLoadFile("data/shaders/terrain_vertex.cso");
    file_t FragFile = PlatformLoadFile("data/shaders/terrain_pixel.cso");
    
    pipeline_create_info PipelineInfo = {};
    PipelineInfo.VertexData          = GetFileBuffer(VertFile);
    PipelineInfo.PixelData           = GetFileBuffer(FragFile);
    PipelineInfo.VertexDataSize      = PlatformGetFileSize(VertFile);
    PipelineInfo.PixelDataSize       = PlatformGetFileSize(FragFile);
    
    pipeline_layout_create_info LayoutInfo[3] = {
        
        { "POSITION", 0, InputFormat_R32G32_FLOAT,    0, 0,  true, 0 }, // size = 8
        { "NORMAL",   0, InputFormat_R32G32B32_FLOAT, 0, 8,  true, 0 }, // size = 12
        { "TEXCOORD", 0, InputFormat_R32G32_FLOAT,    0, 20, true, 0 }, // size = 8
    };
    
    PipelineInfo.PipelineLayout      = LayoutInfo;
    PipelineInfo.PipelineLayoutCount = 3;
    
    Terrain->Pipeline = CreateResource(GlobalResourceRegistry, Resource_Pipeline, &PipelineInfo);
    
    PlatformCloseFile(VertFile);
    PlatformCloseFile(FragFile);
}

file_internal void CreateHeightmap(asset_terrain *Terrain, terrain_settings *Settings)
{
    //~ Run the heightmap generation
    
    // TODO(Dustin): Have SimulateNoise take in the heightmap pointer,
    // rather than always returning an allocated array
    // run the simplex noise simulation
    SimplexNoise::NoiseOctaveSimulation noise_sim = {};
    noise_sim.Allocator     = &GlobalMemory;
    noise_sim.Width         = Settings->HeightmapHeight;
    noise_sim.Height        = Settings->HeightmapWidth;
    noise_sim.NumOctaves    = Settings->NumberOfOctaves;
    noise_sim.Persistence   = Settings->Persistence;
    noise_sim.Low           = Settings->Low;
    noise_sim.High          = Settings->High;
    noise_sim.Exp           = Settings->Exp;
    noise_sim.Dim           = noise_sim.TWODIMENSION;
    r32 *heightmap = SimplexNoise::SimulateNoise( noise_sim );
    
    // run thermal, if active
    if (Settings->ThermalEnabled)
    {
        Erosion::ThermalErosionSimlation simulation;
        simulation.Width            = Settings->HeightmapWidth;
        simulation.Height           = Settings->HeightmapHeight;
        simulation.NumberIterations = Settings->ThermalNumIterations;
        simulation.NoiseMap         = heightmap;
        
        Erosion::SimulateThermalErosion(simulation);
    }
    
    // run inverse thermal, if active
    if (Settings->InverseThermalEnabled)
    {
        Erosion::ThermalErosionSimlation simulation;
        simulation.Width            = Settings->HeightmapWidth;
        simulation.Height           = Settings->HeightmapHeight;
        simulation.NumberIterations = Settings->InverseThermalNumIterations;
        simulation.NoiseMap         = heightmap;
        
        Erosion::SimulateInverseThermalErosion(simulation);
    }
    
    // run hydraulic, if active
    if (Settings->HydraulicEnabled)
    {
        Erosion::ErosionCoefficient coeffErosion;
        coeffErosion.Kr = Settings->RainConstant;
        coeffErosion.Ks = Settings->SolubilityConstant;
        coeffErosion.Ke = Settings->EvaporationCoefficient;
        coeffErosion.Kc = Settings->SedimentTransferMaxCoefficient;
        
        // Create Hydraulic Erosion Simulation Struct
        Erosion::HydraulicErosionSimulation simulation;
        simulation.Allocator        = &GlobalMemory;
        simulation.ErosionCoeffStruct = coeffErosion;
        simulation.Width              = Settings->HeightmapWidth;
        simulation.Height             = Settings->HeightmapHeight;
        simulation.NoiseMap           = heightmap;
        simulation.NumberIterations   = Settings->HydraulicNumIterations;
        
        Erosion::SimulateHydraulicErosion(simulation);
    }
    
    Terrain->HeightmapWidth  = Settings->HeightmapWidth;
    Terrain->HeightmapHeight = Settings->HeightmapHeight;
    Terrain->Heightmap       = heightmap;
    
    //~ Create the texture
    texture2d_create_info TextureInfo = {};
    TextureInfo.Width               = Settings->HeightmapWidth;
    TextureInfo.Height              = Settings->HeightmapHeight;
    TextureInfo.Stride              = 4; // 1 component, 32 bit
    TextureInfo.Usage               = BufferUsage_Immutable;
    TextureInfo.CpuAccessFlags      = BufferCpuAccess_None;
    TextureInfo.BindFlags           = BufferBind_ShaderResource;
    TextureInfo.MiscFlags           = BufferMisc_None;
    TextureInfo.StructureByteStride = 0;
    TextureInfo.Format              = InputFormat_R32_FLOAT;
    
    // Optional data
    TextureInfo.Data             = Terrain->Heightmap;
    TextureInfo.SysMemPitch      = Settings->HeightmapWidth * Settings->HeightmapHeight * TextureInfo.Stride;
    TextureInfo.SysMemSlicePitch = 0;
    
    Terrain->HeightmapTexture = CreateResource(GlobalResourceRegistry, Resource_Texture2D, &TextureInfo);
    
    //~ Create the colormap and its texture
    
    u32 Length = Settings->HeightmapWidth * Settings->HeightmapHeight;
    vec3 *Colormap = palloc<vec3>(&GlobalMemory, Length);
    
    for (u32 i = 0; i < Length; ++i)
    {
        u8 BiomeColor[3];
        Biome::GetBiomeColor(Terrain->Heightmap[i], BiomeColor);
        
        Colormap[i].x = (r32)BiomeColor[0] / 255.0f;
        Colormap[i].y = (r32)BiomeColor[1] / 255.0f;
        Colormap[i].z = (r32)BiomeColor[2] / 255.0f;
    }
    
    Terrain->Colormap = Colormap;
    
    TextureInfo = {};
    TextureInfo.Width               = Settings->HeightmapWidth;
    TextureInfo.Height              = Settings->HeightmapHeight;
    TextureInfo.Stride              = 12; // 3 component, 32 bit
    TextureInfo.Usage               = BufferUsage_Immutable;
    TextureInfo.CpuAccessFlags      = BufferCpuAccess_None;
    TextureInfo.BindFlags           = BufferBind_ShaderResource;
    TextureInfo.MiscFlags           = BufferMisc_None;
    TextureInfo.StructureByteStride = 0;
    TextureInfo.Format              = InputFormat_R32G32B32_FLOAT;
    
    // Optional data
    TextureInfo.Data             = Terrain->Colormap;
    TextureInfo.SysMemPitch      = Settings->HeightmapWidth * Settings->HeightmapHeight * TextureInfo.Stride;
    TextureInfo.SysMemSlicePitch = 0;
    
    Terrain->ColormapTexture = CreateResource(GlobalResourceRegistry, Resource_Texture2D, &TextureInfo);
    
    //~ Create the normal map
    
    Terrain->Normalmap = palloc<vec3>(&GlobalMemory, Length);
    u32 Idx = 0;
    
    for (u32 Row = 0; Row < Settings->HeightmapHeight; ++Row)
    {
        for (u32 Col = 0; Col < Settings->HeightmapWidth; ++Col)
        {
            vec3 Normal = { 0.0f, 1.0f, 0.0f };
            
            if (Row > 0 && Row < Settings->HeightmapHeight - 1 && Col > 0 && Col < Settings->HeightmapWidth - 1)
            {
                u32 i0 = (Row + 0) * Settings->HeightmapWidth + (Col + 0);
                u32 i1 = (Row + 0) * Settings->HeightmapWidth + (Col - 1);
                u32 i2 = (Row + 1) * Settings->HeightmapWidth + (Col + 0);
                u32 i3 = (Row + 0) * Settings->HeightmapWidth + (Col + 1);
                u32 i4 = (Row - 1) * Settings->HeightmapWidth + (Col + 0);
                
#if 0
                vec3 p0 = { (r32)Col + 0, Terrain->Heightmap[i0], (r32)Row + 0 };
                vec3 p1 = { (r32)Col - 1, Terrain->Heightmap[i1], (r32)Row + 0 };
                vec3 p2 = { (r32)Col + 0, Terrain->Heightmap[i2], (r32)Row + 1 };
                vec3 p3 = { (r32)Col + 1, Terrain->Heightmap[i3], (r32)Row + 0 };
                vec3 p4 = { (r32)Col + 0, Terrain->Heightmap[i4], (r32)Row - 1 };
#else
                vec3 p0 = { (r32)Row + 0, Terrain->Heightmap[i0], (r32)Col + 0 };
                vec3 p1 = { (r32)Row - 1, Terrain->Heightmap[i1], (r32)Col + 0 };
                vec3 p2 = { (r32)Row + 0, Terrain->Heightmap[i2], (r32)Col + 1 };
                vec3 p3 = { (r32)Row + 1, Terrain->Heightmap[i3], (r32)Col + 0 };
                vec3 p4 = { (r32)Row + 0, Terrain->Heightmap[i4], (r32)Col - 1 };
#endif
                
                vec3 v1 = norm(p1 - p0);
                vec3 v2 = norm(p2 - p0);
                vec3 v3 = norm(p3 - p0);
                vec3 v4 = norm(p4 - p0);
                
                vec3 v12 = norm(cross(v1, v2));
                vec3 v23 = norm(cross(v2, v3));
                vec3 v34 = norm(cross(v3, v4));
                vec3 v41 = norm(cross(v4, v1));
                
                Normal = norm(v12 + v23 + v34 + v41);
            }
            
            Terrain->Normalmap[Idx++] = Normal;
        }
    }
    
    TextureInfo = {};
    TextureInfo.Width               = Settings->HeightmapWidth;
    TextureInfo.Height              = Settings->HeightmapHeight;
    TextureInfo.Stride              = 12; // 3 component, 32 bit
    TextureInfo.Usage               = BufferUsage_Immutable;
    TextureInfo.CpuAccessFlags      = BufferCpuAccess_None;
    TextureInfo.BindFlags           = BufferBind_ShaderResource;
    TextureInfo.MiscFlags           = BufferMisc_None;
    TextureInfo.StructureByteStride = 0;
    TextureInfo.Format              = InputFormat_R32G32B32_FLOAT;
    
    // Optional data
    TextureInfo.Data             = Terrain->Normalmap;
    TextureInfo.SysMemPitch      = Settings->HeightmapWidth * Settings->HeightmapHeight * TextureInfo.Stride;
    TextureInfo.SysMemSlicePitch = 0;
    
    Terrain->NormalmapTexture = CreateResource(GlobalResourceRegistry, Resource_Texture2D, &TextureInfo);
    
}
