
file_internal void CreateHeightmap(asset_terrain *Terrain, terrain_settings *Settings);


void CreateTerrain(asset_terrain *Terrain, terrain_settings *Settings)
{
#if 0
    GenerateTerrainGrid(Terrain, Settings->TerrainWidth, Settings->TerrainHeight);
    CreateTerrainVertexBuffers(Terrain);
    CreateTerrainMaterial(Terrain);
#endif
    
    CreateHeightmap(Terrain, Settings);
}

void ResetTerrain(asset_terrain *Terrain, terrain_settings *Settings)
{
}

void DestroyTerrain(asset_terrain *Terrain)
{
}

// SOURCE: https://stackoverflow.com/questions/5915753/generate-a-plane-with-triangle-strips
void GenerateTerrainGrid(asset_terrain *Terrain, u32 width, u32 height)
{
    Terrain->Width       = width;
    Terrain->Height      = height;
    
    Terrain->VertexCount = (width) * (height);
    Terrain->IndexCount = (width * height) + (width - 1) * (width - 2);
    
    Terrain->Vertices = palloc<terrain_vertex>(&GlobalMemory, Terrain->VertexCount);
    Terrain->Indices = palloc<u32>(&GlobalMemory, Terrain->IndexCount);
    
    // Generate the grid information
    // Create the vertex list
    u64 idx = 0;
    for (u32 r = 0; r < height; ++r)
    {
        u32 base = r * width;
        for (u32 c = 0; c < width; ++c, idx += 6)
        {
            Terrain->Vertices[base + c].Position = {(r32)c, (r32)r};
            Terrain->Vertices[base + c].Uvs      = {c / (r32)width, r / (r32)height};
            Terrain->Vertices[base + c].Normal   = {0.0f, 0.0f, 0.0f}; // TODO(Dustin)
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
    IndexBufferInfo.Size                = Terrain->VertexCount * sizeof(terrain_vertex);
    IndexBufferInfo.Usage               = BufferUsage_Default;
    IndexBufferInfo.CpuAccessFlags      = BufferCpuAccess_None;
    IndexBufferInfo.BindFlags           = BufferBind_IndexBuffer;
    IndexBufferInfo.MiscFlags           = BufferMisc_None;
    IndexBufferInfo.StructureByteStride = sizeof(terrain_vertex);
    
    // Optional data
    IndexBufferInfo.Data             = Terrain->Vertices;
    IndexBufferInfo.SysMemPitch      = IndexBufferInfo.Size;
    IndexBufferInfo.SysMemSlicePitch = 0;
    
    Terrain->IndexBuffer = CreateResource(GlobalResourceRegistry, Resource_Buffer, &IndexBufferInfo);
    
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
        
        { "POSITION", 0, InputFormat_R32G32_FLOAT,    0,  0, true, 0 }, // size = 8
        { "NORMAL",   0, InputFormat_R32G32B32_FLOAT, 8,  0, true, 0 }, // size = 12
        { "TEXCOORD", 0, InputFormat_R32G32_FLOAT,    20, 0, true, 0 }, // size = 20
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
    //TextureInfo.Device              = pRenderer->Device;
    TextureInfo.Width               = Settings->HeightmapWidth;
    TextureInfo.Height              = Settings->HeightmapHeight;
    TextureInfo.Stride              = 4; // 1 component, 32 bit
    TextureInfo.Usage               = BufferUsage_Immutable;
    TextureInfo.CpuAccessFlags      = BufferCpuAccess_None;
    TextureInfo.BindFlags           = BufferBind_ShaderResource;
    TextureInfo.MiscFlags           = BufferMisc_None;
    TextureInfo.StructureByteStride = 0;
    TextureInfo.Format              = InputFormat_R32_FLOAT;
    Terrain->HeightmapTexture = CreateResource(GlobalResourceRegistry, Resource_Texture2D, &TextureInfo);
    
    // upload the texture data
    // TODO(Dustin): Interface for uploading data to the gpu
#if 0
    ID3D11DeviceContext *DeviceContext = GlobalRenderer->DeviceContext;
    ID3D11Texture2D *Texture = GlobalRegistry->Resources[Terrain->HeightmapTexture.Id.Index]->Texture.Handle;
    
    D3D11_MAPPED_SUBRESOURCE TextureResource;
    HRESULT hr = DeviceContext->Map(Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &TextureResource);
    
    if (FAILED(hr))
    {
        mprinte("Failed to map the heightmap texture resource %d!\n", hr);
    }
    else
    {
        void* Backbuffer = TextureResource.pData;
        memcpy(Backbuffer, heightmap, TextureResource.DepthPitch);
        DeviceContext->Unmap(Texture, 0);
    }
#endif
}
