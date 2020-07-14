
file_internal void LoadPrimitives(mesh_loader *Loader);
file_internal void LoadMeshes(mesh_loader *Loader);
file_internal void LoadNodes(mesh_loader *Loader);
file_internal void LoadVertexData(mesh_loader *Loader);

file_internal void LoadPrimitives(mesh_loader *Loader)
{
}

file_internal void LoadMeshes(mesh_loader *Loader)
{
}

file_internal void LoadNodes(mesh_loader *Loader)
{
}

file_internal void LoadVertexData(mesh_loader *Loader)
{
}

void LoadMesh(const char *Filename, free_allocator *Allocator)
{
    asset_model Model = {};
    
    mesh_loader Loader = {};
    Mstring(&Loader.Filename, Filename, strlen(Filename));
    Loader.Asset = &Model;
    
    file_t ModelFile = PlatformLoadFile(Filename);
    
    PlatformReadFile(ModelFile, &Loader.BinaryFilename, 1, "m");
    
    //~ Parse the primitives
    PlatformReadFile(ModelFile, &Loader.Asset->PrimitivesCount, 1, "i32");
    Loader.Asset->Primitives = palloc<primitive>(Allocator, Loader.Asset->PrimitivesCount);
    
    LoadPrimitives(&Loader);
    
    //~ Parse the Meshes
    PlatformReadFile(ModelFile, &Loader.Asset->MeshesCount, 1, "i32");
    Loader.Asset->Meshes = palloc<mesh>(Allocator, Loader.Asset->MeshesCount);
    
    LoadMeshes(&Loader);
    
    //~ Parse the Scene Nodes
    PlatformReadFile(ModelFile, &Loader.Asset->NodesCount, 1, "i32");
    Loader.Asset->Nodes = palloc<model_node>(Allocator, Loader.Asset->NodesCount);
    
    LoadNodes(&Loader);
    
    //~ Parse the Models (+ scenes)
    i32 SceneCount;
    PlatformReadFile(ModelFile, &SceneCount, 1, "i32");
    
    // Determine the total amount of models in the file
    i32 ModelCount = 0;
    PlatformFileSetSavepoint(ModelFile);
    {
        for (i32 SceneIdx = 0; SceneIdx < SceneCount; SceneIdx++)
        {
            u32 NumDisjoint;
            PlatformReadFile(ModelFile, &NumDisjoint, 1, "u32");
            ModelCount += NumDisjoint;
            
            for (u32 di = 0; di < NumDisjoint; di++)
            {
                i32 Dummy;
                PlatformReadFile(ModelFile, &Dummy, 1, "i32");
            }
        }
    }
    PlatformFileRestoreSavepoint(ModelFile);
    
    Loader.Asset->RootModelNodesCount = ModelCount;
    Loader.Asset->RootModelNodes = palloc<model_node*>(Allocator, ModelCount);
    
    for (i32 SceneIdx = 0; SceneIdx < SceneCount; SceneIdx++)
    {
        u32 NumDisjoint;
        PlatformReadFile(ModelFile, &NumDisjoint, 1, "u32");
        ModelCount += NumDisjoint;
        
        for (u32 di = 0; di < NumDisjoint; di++)
        {
            i32 NodeIdx;
            PlatformReadFile(ModelFile, &NodeIdx, 1, "i32");
            Loader.Asset->RootModelNodes[di] = Loader.Asset->Nodes + NodeIdx;
        }
    }
    
    file_t BinaryFile = PlatformOpenFile(GetStr(&Loader.BinaryFilename));
    
    
    PlatformCloseFile(ModelFile);
    PlatformCloseFile(BinaryFile);
}
