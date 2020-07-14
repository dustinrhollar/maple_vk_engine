
void InitTerrain(terrain_mesh *terrain)
{
}

void DestroyTerrain(terrain_mesh *terrain)
{
}

// SOURCE: https://stackoverflow.com/questions/5915753/generate-a-plane-with-triangle-strips
void GenerateTerrainGrid(terrain_mesh *terrain, u32 width, u32 height)
{
    terrain->Width       = width;
    terrain->Height      = height;
    
    terrain->VertexCount = (width) * (height);
    terrain->IndexCount = (width * height) + (width - 1) * (width - 2);
    
#if 0
    terrain->Vertices = palloc<TerrainVertex>(terrain->VertexCount);
    terrain->Indices = palloc<u32>(terrain->IndexCount);
    
    // Generate the grid information
    // Create the vertex list
    u64 idx = 0;
    for (u32 r = 0; r < height; ++r)
    {
        u32 base = r * width;
        for (u32 c = 0; c < width; ++c, idx += 6)
        {
            terrain->Vertices[base + c].Position = {(r32)c, (r32)r};
            terrain->Vertices[base + c].Uvs      = {c / (r32)width, r / (r32)height};
            terrain->Vertices[base + c].Normal   = {0.0f, 0.0f, 0.0f}; // TODO(Dustin)
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
                terrain->Indices[i++] = (r + 0) * width + c;
                terrain->Indices[i++] = (r + 1) * width + c;
            }
        }
        else
        {
            for (u32 c = width-1; c > 0; c--)
            {
                terrain->Indices[i++] = c + (r + 1) * width;
                terrain->Indices[i++] = c - 1 + (r + 0) * width;
            }
        }
        
        if ((height & 1) && height > 2)
        {
            terrain->Indices[i++] = (height - 1) * width;
        }
    }
#endif
}

void CreateTerrainVertexBuffers(terrain_mesh *terrain)
{
}

void CreateTerrainMaterial(terrain_mesh *terrain)
{
}

// Create the heightmap texture
void AttachHeightmapToTerrain(terrain_mesh *terrain, r32 *heightmap, u32 width, u32 height)
{
    //terrain->Heightmap = heightmap;
}

void RegenerateTerrain(terrain_mesh *terrain, terrain_settings &settings)
{
    // Recreate the base mesh
#if 0
    if (settings.TerrainMeshUpdated)
    {
        // re-generate the terrain mesh
        pfree(terrain->Vertices);
        pfree(terrain->Indices);
        
        vk::DestroyVmaBuffer(terrain->VertexBuffer.Handle, terrain->VertexBuffer.Memory);
        vk::DestroyVmaBuffer(terrain->IndexBuffer.Handle, terrain->IndexBuffer.Memory);
        
        GenerateTerrainGrid(terrain, settings.TerrainWidth, settings.TerrainHeight);
        CreateTerrainVertexBuffers(terrain, command_pool);
        
        settings.TerrainMeshUpdated = false;
    }
#endif
    
    // TODO(Dustin): Have SimulateNoise take in the heightmap pointer,
    // rather than always returning an allocated array
    // run the simplex noise simulation
    SimplexNoise::NoiseOctaveSimulation noise_sim = {};
    noise_sim.Width         = settings.HeightmapHeight;
    noise_sim.Height        = settings.HeightmapWidth;
    noise_sim.NumOctaves    = settings.NumberOfOctaves;
    noise_sim.Persistence   = settings.Persistence;
    noise_sim.Low           = settings.Low;
    noise_sim.High          = settings.High;
    noise_sim.Exp           = settings.Exp;
    noise_sim.Dim           = noise_sim.TWODIMENSION;
    r32 *heightmap = SimplexNoise::SimulateNoise( noise_sim );
    
    // run thermal, if active
    if (settings.ThermalEnabled)
    {
        Erosion::ThermalErosionSimlation simulation;
        simulation.Width            = settings.HeightmapWidth;
        simulation.Height           = settings.HeightmapHeight;
        simulation.NumberIterations = settings.ThermalNumIterations;
        simulation.NoiseMap         = heightmap;
        
        Erosion::SimulateThermalErosion(simulation);
    }
    
    // run inverse thermal, if active
    if (settings.InverseThermalEnabled)
    {
        Erosion::ThermalErosionSimlation simulation;
        simulation.Width            = settings.HeightmapWidth;
        simulation.Height           = settings.HeightmapHeight;
        simulation.NumberIterations = settings.InverseThermalNumIterations;
        simulation.NoiseMap         = heightmap;
        
        Erosion::SimulateInverseThermalErosion(simulation);
    }
    
    // run hydraulic, if active
    if (settings.HydraulicEnabled)
    {
        Erosion::ErosionCoefficient coeffErosion;
        coeffErosion.Kr = settings.RainConstant;
        coeffErosion.Ks = settings.SolubilityConstant;
        coeffErosion.Ke = settings.EvaporationCoefficient;
        coeffErosion.Kc = settings.SedimentTransferMaxCoefficient;
        
        // Create Hydraulic Erosion Simulation Struct
        Erosion::HydraulicErosionSimulation simulation;
        simulation.ErosionCoeffStruct = coeffErosion;
        simulation.Width              = settings.HeightmapWidth;
        simulation.Height             = settings.HeightmapHeight;
        simulation.NoiseMap           = heightmap;
        simulation.NumberIterations   = settings.HydraulicNumIterations;
        
        Erosion::SimulateHydraulicErosion(simulation);
    }
    
    // re-create the heightmap attachment process
#if 0
    vk::DestroyImageSampler(terrain->HeightmapImage.Sampler);
    vk::DestroyImageView(terrain->HeightmapImage.View);
    vk::DestroyVmaImage(terrain->HeightmapImage.Handle, terrain->HeightmapImage.Memory);
    pfree(terrain->Heightmap);
    
    AttachHeightmapToTerrain(terrain, heightmap, settings.HeightmapWidth, settings.HeightmapWidth,
                             command_pool);
    
    settings.HeightmapUpdated   = false;
#endif
}


void RenderTerrain(terrain_mesh *terrain, bool is_wireframe)
{
}