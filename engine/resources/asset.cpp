namespace masset
{
    // TODO(Dustin): After memory manager has been refactored, have a pool here for
    // model memory rather than just allocating from global storage.
    
    file_global const u32              MaxAssets = 100;
    file_global mm::resource_allocator AssetAllocator;
    
    struct asset_registry
    {
        asset **Assets;
        u32     NextIdx;
        u32     Count;
        u32     Cap;
    } file_global AssetRegistry;
    
    struct mesh_loader
    {
        cgltf_data   *Data;
        jstring       Filename;
        jstring       BinaryFilename;
        frame_params *FrameParams;
        
        FileBuffer    Buffer;
        char         *BinaryData;
        asset_model  *Asset;
    };
    
    file_internal void InitRegistry(asset_registry *Registry, u32 Cap);
    file_internal void FreeRegistry(asset_registry *Registry);
    file_internal void RegistryResize(asset_registry *Registry, u32 NewSize);
    file_internal asset_id_t RegistryAdd(asset_registry *Registry, asset Asset);
    file_internal void RegistryRemove(asset_registry *Registry, u32 AssetIdx);
    file_internal inline bool RegistryIsValidAsset(asset_registry *Registry, asset_id_t Id);
    
    file_internal void FreeAsset(asset *Asset);
    //file_internal void FreeModel(model Model);
    file_internal void ShutdownNode(model_node *Node);
    file_internal void ShutdownMesh(mesh *Mesh);
    file_internal void LoadPrimitives(mesh_loader *Loader);
    file_internal void LoadMeshes(mesh_loader *Loader);
    file_internal void LoadNodes(mesh_loader *Loader);
    file_internal asset LoadModel(frame_params *FrameParams, jstring Filename);
    
    
    //~ Asset Registry functionality
    
    file_internal void InitRegistry(asset_registry *Registry, u32 Cap)
    {
        Registry->NextIdx = 0;
        Registry->Count   = 0;
        Registry->Cap     = Cap;
        Registry->Assets  = palloc<asset*>(Registry->Cap);
        
        mm::InitResourceAllocator(&AssetAllocator, sizeof(asset), Cap);
        
        for (u32 i = 0; i < Registry->Cap; ++i)
            Registry->Assets[i] = nullptr;
    }
    
    file_internal void FreeRegistry(asset_registry *Registry)
    {
        for (u32 i = 0; i < Registry->Cap; ++i)
        {
            if (Registry->Assets[i])
            {
                FreeResource(Registry->Assets[i]);
                Registry->Assets[i] = nullptr;
            }
        }
        
        pfree(Registry->Assets);
        Registry->Count   = 0;
        Registry->Cap     = 0;
        Registry->NextIdx = 0;
    }
    
    file_internal void RegistryResize(asset_registry *Registry, u32 NewSize)
    {
        asset **NewRegistry = palloc<asset*>(NewSize);
        
        for (u32 Idx = 0; Idx < Registry->Count; Idx++)
            NewRegistry[Idx] = Registry->Assets[Idx];
        
        // Mark empty slots as invalid resources
        for (u32 Idx = Registry->Count; Idx < NewSize; ++Idx)
            NewRegistry[Idx] = nullptr;
        
        pfree(Registry->Assets);
        Registry->Assets = NewRegistry;
        Registry->Cap    = NewSize;
    }
    
    file_internal asset_id_t RegistryAdd(asset_registry *Registry, asset Asset)
    {
        if (Registry->NextIdx + 1 >= Registry->Cap)
            RegistryResize(Registry, (Registry->Cap>0) ? Registry->Cap*2 : 20);
        
        asset *NewAsset = (asset*)ResourceAllocatorAlloc(&AssetAllocator);
        *NewAsset = Asset;
        
        asset_id_t Result = Registry->NextIdx;
        NewAsset->Id = Result;
        Registry->Assets[Registry->NextIdx++] = NewAsset;
        Registry->Count++;
        
        return Result;
    }
    
    file_internal void RegistryRemove(asset_registry *Registry, u32 AssetIdx)
    {
        asset *Asset = Registry->Assets[AssetIdx];
        FreeAsset(Asset);
        ResourceAllocatorFree(&AssetAllocator, Asset);
        Registry->Assets[AssetIdx]->Id = -1;
        Registry->Count--;
    }
    
    file_internal inline bool RegistryIsValidAsset(asset_registry *Registry,
                                                   asset_id_t Id)
    {
        return Id < Registry->Count && Registry->Assets[Id] && Registry->Assets[Id]->Id == Id;
    }
    
    
    //~  Model Asset Functionality
    
    file_internal void LoadVertexData(mesh_loader *Loader)
    {
        for (i32 PrimitiveIdx = 0; PrimitiveIdx < Loader->Asset->PrimitivesCount; PrimitiveIdx++)
        {
            primitive *Primitive = &Loader->Asset->Primitives[PrimitiveIdx];
            
            if (Primitive->IsIndexed)
            {
                index_buffer_create_info *IndexBufferCreatInfo = talloc<index_buffer_create_info>(1);
                IndexBufferCreatInfo->Size = Primitive->IndexStride * Primitive->IndexCount;
                IndexBufferCreatInfo->Data = (char*)Loader->BinaryData + Primitive->IndicesOffset;
                
                Primitive->IndexBuffer  = mresource::Load(Loader->FrameParams,
                                                          Resource_IndexBuffer, IndexBufferCreatInfo);
            }
            
            vertex_buffer_create_info *VertexBufferCreatInfo = talloc<vertex_buffer_create_info>(1);
            VertexBufferCreatInfo->Size = Primitive->VertexStride * Primitive->VertexCount;
            VertexBufferCreatInfo->Data = (char*)Loader->BinaryData + Primitive->VerticesOffset;
            
            Primitive->VertexBuffer = mresource::Load(Loader->FrameParams,
                                                      Resource_VertexBuffer,
                                                      VertexBufferCreatInfo);
        }
    }
    
    file_internal void LoadPrimitives(mesh_loader *Loader)
    {
        for (i32 PrimitiveIdx = 0; PrimitiveIdx < Loader->Asset->PrimitivesCount; PrimitiveIdx++)
        {
            /*
Primitive List has the following format:
---- u64  -> base Offset into the Data Block
 ---- u32  -> Number of Indices
---- u32  -> Index Stride
---- u32  -> Number of Vertices
---- u32  -> Vertex Stride
---- bool -> whether or not it is a skinned vertex
---- vec3 -> Minimum vertex position..
 ---- vec3 -> Maximum vertex position

*/
            primitive Primitive = {};
            
            // Data info
            u64 Offset;
            ReadUInt64FromBinaryBuffer(&Loader->Buffer, &Offset);
            ReadUInt32FromBinaryBuffer(&Loader->Buffer, &Primitive.IndexCount);
            ReadUInt32FromBinaryBuffer(&Loader->Buffer, &Primitive.IndexStride);
            ReadUInt32FromBinaryBuffer(&Loader->Buffer, &Primitive.VertexCount);
            ReadUInt32FromBinaryBuffer(&Loader->Buffer, &Primitive.VertexStride);
            ReadBoolFromBinaryBuffer(&Loader->Buffer, &Primitive.IsSkinned);
            
            // Min/Max data for the primitve
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Primitive.Min.x);
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Primitive.Min.y);
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Primitive.Min.z);
            
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Primitive.Max.x);
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Primitive.Max.y);
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Primitive.Max.z);
            
            Primitive.IndicesOffset  = Offset;
            Primitive.VerticesOffset = Primitive.IndicesOffset + Primitive.IndexCount * Primitive.IndexStride;
            
            Primitive.IsIndexed = Primitive.IndexCount > 0;
            
            Loader->Asset->Primitives[PrimitiveIdx] = Primitive;
        }
    }
    
    file_internal void LoadMeshes(mesh_loader *Loader)
    {
        for (i32 MeshIdx = 0; MeshIdx < Loader->Asset->MeshesCount; ++MeshIdx)
        {
            /*
    Mesh List
    ---- str  -> Mesh Name
    ---- u64  -> Number of Primitives
    ---- Primitive Idx List
    -------- i32 -> Index into the primitive array
    */
            
            mesh Mesh = {};
            
            ReadJStringFromBinaryBuffer(&Loader->Buffer, &Mesh.Name);
            ReadUInt64FromBinaryBuffer(&Loader->Buffer, &Mesh.PrimitivesCount);
            Mesh.Primitives = palloc<primitive*>(Mesh.PrimitivesCount);
            
            for (i64 PrimIdx = 0; PrimIdx < Mesh.PrimitivesCount; PrimIdx++)
            {
                i32 Idx;
                ReadInt32FromBinaryBuffer(&Loader->Buffer, &Idx);
                
                Mesh.Primitives[PrimIdx] = Loader->Asset->Primitives + Idx;
            }
            
            Loader->Asset->Meshes[MeshIdx] = Mesh;
        }
    }
    
    file_internal void LoadNodes(mesh_loader *Loader)
    {
        for (i32 NodeIdx = 0; NodeIdx < Loader->Asset->NodesCount; NodeIdx++)
        {
            /*
Node List
---- str  -> Name of the node
---- str  -> Name of the mesh. If one is not attached, then a single u32 with value of 0 has been written
---- i32  -> Index of the parent node
---- u64  -> Number of Children the node has
---- Children Index List
-------- i32 -> Index into the Node array
---- vec3 -> vector for translation
---- vec3 -> vector for scale
---- vec4 -> vector for rotation
*/
            
            model_node Node = {};
            
            ReadJStringFromBinaryBuffer(&Loader->Buffer, &Node.Name);
            
            i32 Idx;
            ReadInt32FromBinaryBuffer(&Loader->Buffer, &Idx);
            
            if (Idx != -1)
                Node.Parent = Loader->Asset->Nodes + Idx;
            
            else
                Node.Parent = nullptr;
            
            ReadUInt64FromBinaryBuffer(&Loader->Buffer, &Node.ChildrenCount);
            Node.Children = palloc<model_node*>(Node.ChildrenCount);
            
            for (u32 ChildIdx = 0; ChildIdx < Node.ChildrenCount; ++ChildIdx)
            {
                ReadInt32FromBinaryBuffer(&Loader->Buffer, &Idx);
                Node.Children[ChildIdx] = Loader->Asset->Nodes + Idx;
            }
            
            
            // Tranlation vec
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Node.Translation.x);
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Node.Translation.y);
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Node.Translation.z);
            
            // Scaling
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Node.Scale.x);
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Node.Scale.y);
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Node.Scale.z);
            
            // Quaternion Rotation
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Node.Rotation.x);
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Node.Rotation.y);
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Node.Rotation.z);
            ReadFloatFromBinaryBuffer(&Loader->Buffer, &Node.Rotation.w);
            
            // NOTE(Dustin): Remove this?
            ReadInt32FromBinaryBuffer(&Loader->Buffer, &Idx);
            if (Idx != -1)
                Node.Mesh = Loader->Asset->Meshes + Idx;
            
            else
                Node.Mesh = nullptr;
            
            Loader->Asset->Nodes[NodeIdx] = Node;
        }
    }
    
    file_internal asset LoadModel(frame_params *FrameParams, jstring Filename)
    {
        
        asset ModelAsset = {};
        ModelAsset.Type = Asset_Model;
        
        mesh_loader Loader = {};
        Loader.Filename    = Filename;
        Loader.FrameParams = FrameParams;
        Loader.Asset       = &ModelAsset.Model;
        
        jstring ModelFile = PlatformLoadFile(Filename);
        
        Loader.Buffer.start = ModelFile.GetCStr();
        Loader.Buffer.brkp  = Loader.Buffer.start;
        Loader.Buffer.cap   = ModelFile.len;
        
        ReadJStringFromBinaryBuffer(&Loader.Buffer, &Loader.BinaryFilename);
        
        //~ Parse the primitives
        ReadInt32FromBinaryBuffer(&Loader.Buffer, &Loader.Asset->PrimitivesCount);
        Loader.Asset->Primitives = palloc<primitive>(Loader.Asset->PrimitivesCount);
        
        LoadPrimitives(&Loader);
        
        //~ Parse the Meshes
        ReadInt32FromBinaryBuffer(&Loader.Buffer, &Loader.Asset->MeshesCount);
        Loader.Asset->Meshes = palloc<mesh>(Loader.Asset->MeshesCount);
        
        LoadMeshes(&Loader);
        
        //~ Parse the Scene Nodes
        ReadInt32FromBinaryBuffer(&Loader.Buffer, &Loader.Asset->NodesCount);
        Loader.Asset->Nodes = palloc<model_node>(Loader.Asset->NodesCount);
        
        LoadNodes(&Loader);
        
        //~ Parse the Models
        i32 SceneCount;
        ReadInt32FromBinaryBuffer(&Loader.Buffer, &SceneCount);
        
        char *Rewind = Loader.Buffer.brkp;
        
        i32 ModelCount = 0;
        for (i32 SceneIdx = 0; SceneIdx < SceneCount; SceneIdx++)
        {
            u32 NumDisjoint;
            ReadUInt32FromBinaryBuffer(&Loader.Buffer, &NumDisjoint);
            ModelCount += NumDisjoint;
            
            for (u32 di = 0; di < NumDisjoint; di++)
            {
                i32 Dummy;
                ReadInt32FromBinaryBuffer(&Loader.Buffer, &Dummy);
            }
        }
        Loader.Buffer.brkp = Rewind;
        
        Loader.Asset->RootModelNodesCount = ModelCount;
        Loader.Asset->RootModelNodes = palloc<model_node*>(ModelCount);
        
        for (i32 SceneIdx = 0; SceneIdx < SceneCount; SceneIdx++)
        {
            u32 NumDisjoint;
            ReadUInt32FromBinaryBuffer(&Loader.Buffer, &NumDisjoint);
            
            for (u32 di = 0; di < NumDisjoint; di++)
            {
                i32 NodeIdx;
                ReadInt32FromBinaryBuffer(&Loader.Buffer, &NodeIdx);
                
                Loader.Asset->RootModelNodes[di] = Loader.Asset->Nodes + NodeIdx;
            }
        }
        
        //~ Load the data file and upload primitive buffers
        
        jstring BinaryFile = PlatformLoadFile(Loader.BinaryFilename);
        
        Loader.BinaryData = talloc<char>(BinaryFile.len);
        memcpy(Loader.BinaryData, BinaryFile.GetCStr(), BinaryFile.len);
        
        LoadVertexData(&Loader);
        
        //~ Clean Resources
        
        BinaryFile.Clear();
        Loader.BinaryFilename.Clear();
        ModelFile.Clear();
        
        
        return ModelAsset;
    }
    
    asset_id_t Load(asset_type Type, void *Data)
    {
        asset_id_t Result = -1;
        
        asset Asset = {};
        
        switch (Type)
        {
            case Asset_Model:
            {
                model_create_info *Info = static_cast<model_create_info*>(Data);
                Asset = LoadModel(Info->FrameParams, Info->Filename);
            } break;
            
            case Asset_Texture:
            {
            } break;
            
            case Asset_Material:
            {
            } break;
            
            default:
            {
                mprinte("Unknown asset type for load \"%d\"!\n", Type);
            } break;
        }
        
        // TODO(Dustin): Error check?
        Result = RegistryAdd(&AssetRegistry, Asset);
        
        return Result;
    }
    
    void GetModelAssets(asset **Assets, u32 *Count)
    {
        *Assets = talloc<asset>(AssetRegistry.Count);
        
        u32 Iter = 0;
        for (u32 AssetIdx = 0; AssetIdx < AssetRegistry.NextIdx; ++AssetIdx)
        {
            if (AssetRegistry.Assets[AssetIdx])
            {
                (*Assets)[Iter++] = *AssetRegistry.Assets[AssetIdx];
            }
        }
        
        *Count = Iter;
    }
    
    file_internal void ShutdownMesh(mesh *Mesh)
    {
        pfree(Mesh->Primitives);
        Mesh->Primitives      = nullptr;
        Mesh->PrimitivesCount = 0;
        Mesh->Name.Clear();
    }
    
    file_internal void ShutdownNode(model_node *Node)
    {
        if (Node->ChildrenCount > 0)
            pfree(Node->Children);
        
        Node->Children      = nullptr;
        Node->Mesh          = nullptr;
        Node->Parent        = nullptr;
        Node->ChildrenCount = 0;
        Node->Name.Clear();
    }
    
    void Init()
    {
        InitRegistry(&AssetRegistry, 10);
    }
    
    file_internal void FreeAsset(asset *Asset)
    {
        switch (Asset->Type)
        {
            case Asset_Model:
            {
                for (u32 NodeIdx = 0; NodeIdx < Asset->Model.NodesCount; ++NodeIdx)
                    ShutdownNode(&Asset->Model.Nodes[NodeIdx]);
                
                pfree(Asset->Model.Nodes);
                Asset->Model.Nodes = nullptr;
                Asset->Model.NodesCount = 0;
                
                for (u32 MeshIdx = 0; MeshIdx < Asset->Model.MeshesCount; ++MeshIdx)
                    ShutdownMesh(&Asset->Model.Meshes[MeshIdx]);
                
                pfree(Asset->Model.Meshes);
                Asset->Model.Meshes = nullptr;
                Asset->Model.MeshesCount = 0;
                
                pfree(Asset->Model.Primitives);
                Asset->Model.Primitives = nullptr;
                Asset->Model.PrimitivesCount = 0;
                
                pfree(Asset->Model.RootModelNodes);
                Asset->Model.RootModelNodes = nullptr;
                Asset->Model.RootModelNodesCount = 0;
            } break;
            
            case Asset_Texture:
            {
            } break;
            
            case Asset_Material:
            {
            } break;
            
            default:
            {
                mprinte("Unknown asset type for load \"%d\"!\n", Asset->Type);
            } break;
        }
    }
    
    void Free()
    {
        for (u32 Asset = 0; Asset < AssetRegistry.NextIdx; ++Asset)
        {
            if (AssetRegistry.Assets[Asset])
            {
                FreeAsset(AssetRegistry.Assets[Asset]);
            }
        }
        
        mm::FreeResourceAllocator(&AssetAllocator);
    }
    
    
    //~ Material Asset Functionality
    
    
    
    //~ Texture Asset Functionality
    
}; // masset