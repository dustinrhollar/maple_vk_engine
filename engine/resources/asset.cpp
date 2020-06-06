namespace masset
{
    //~ Special List for handling assets id lists
    
    inline bool operator==(asset_id_t &Lhs, asset_id_t &Rhs)
    {
        return Lhs.Type == Rhs.Type && Lhs.Gen == Rhs.Gen && Lhs.Index == Rhs.Index;
    }
    
    struct asset_id_list
    {
        u32 Cap;
        u32 Size;
        asset_id_t *Ptr;
    };
    
    file_internal void AssetIdListInit(asset_id_list *List, u32 InitialCap = 0);
    file_internal void AssetIdListFree(asset_id_list *List);
    file_internal void AssetIdListAdd(asset_id_list *List, asset_id_t AssetId);
    file_internal void AssetIdListRemove(asset_id_list *List, asset_id_t AssetId);
    file_internal void AssetIdListResize(asset_id_list *List, u32 NewCap);
    
    file_internal void AssetIdListInit(asset_id_list *List, u32 InitialCap)
    {
        List->Cap  = (InitialCap>0) ? InitialCap : 10;
        List->Size = 0;
        List->Ptr  = palloc<asset_id_t>(List->Cap);
    }
    
    file_internal void AssetIdListFree(asset_id_list *List)
    {
        if (List->Ptr) pfree(List->Ptr);
        List->Ptr  = nullptr;
        List->Cap  = 0;
        List->Size = 0;
    }
    
    file_internal void AssetIdListAdd(asset_id_list *List, asset_id_t AssetId)
    {
        if (List->Size + 1 > List->Cap) AssetIdListResize(List, List->Cap * 2);
        List->Ptr[List->Size++] = AssetId;
    }
    
    file_internal void AssetIdListRemove(asset_id_list *List, asset_id_t AssetId)
    {
        // TODO(Dustin): Currently does a linear search in the list for the id.
        // probably not the best approach.
        for (u32 Idx = 0; Idx < List->Size; Idx++)
        {
            if (List->Ptr[Idx] == AssetId)
            {
                // swap with the last element, order of this list doesn't matter
                List->Ptr[Idx] = List->Ptr[List->Size - 1];
                --List->Size;
                break;
            }
        }
    }
    
    file_internal void AssetIdListResize(asset_id_list *List, u32 NewCap)
    {
        asset_id_t *Ptr = palloc<asset_id_t>(NewCap);
        for (u32 Id = 0; Id < List->Size; ++Id) Ptr[Id] = List->Ptr[Id];
        pfree(List->Ptr);
        List->Ptr = Ptr;
        List->Cap = NewCap;
    }
    
    //~ Assets functionality
    
    file_global const u32              MaxAssets = 100;
    file_global mm::resource_allocator AssetAllocator;
    
    struct asset_registry
    {
        asset **Assets;
        
        // it would be desirable if I did not have to iterate
        // over all assets in the registry when drawing. So this
        // array contains a list of asset ids so that they can be
        // iterated over.
        asset_id_list ModelAssets;
        
        // Useful for fast lookups for textures and materials
        // necessary when loading materials/textures
        HashTable<jstring, asset_id_t> MaterialMap;
        HashTable<jstring, asset_id_t> TextureMap;
        
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
    file_internal void ShutdownNode(model_node *Node);
    file_internal void ShutdownMesh(mesh *Mesh);
    file_internal void LoadPrimitives(mesh_loader *Loader);
    file_internal void LoadMeshes(mesh_loader *Loader);
    file_internal void LoadNodes(mesh_loader *Loader);
    file_internal asset LoadModel(frame_params *FrameParams, jstring Filename);
    file_internal asset_id_t LoadMaterial(jstring MaterialFilename, jstring MaterialName);
    
    //~ Asset Registry functionality
    
    file_internal asset_id_t GenAssetId(asset_registry *Registry, asset_type Type)
    {
        asset_id_t Result = {};
        
        // TODO(Dustin): Allow for multiple generations
        
        Result.Type  = Type;
        Result.Gen   = 0;
        Result.Index = Registry->NextIdx++;
        
        return Result;
    }
    
    file_internal inline bool RegistryIsValidAsset(asset_registry *Registry,
                                                   asset_id_t Id)
    {
        return Id.Index < Registry->Count && Registry->Assets[Id.Index] && Registry->Assets[Id.Index]->Id.Gen == Id.Gen;
    }
    
    file_internal void InitRegistry(asset_registry *Registry, u32 Cap)
    {
        Registry->NextIdx = 0;
        Registry->Count   = 0;
        Registry->Cap     = Cap;
        Registry->Assets  = palloc<asset*>(Registry->Cap);
        AssetIdListInit(&Registry->ModelAssets);
        
        mm::InitResourceAllocator(&AssetAllocator, sizeof(asset), Cap);
        
        for (u32 i = 0; i < Registry->Cap; ++i)
            Registry->Assets[i] = nullptr;
        
        // NOTE(Dustin): these are not currently resizable, so keep that in mind...
        Registry->MaterialMap = HashTable<jstring, asset_id_t>(50);
        Registry->TextureMap  = HashTable<jstring, asset_id_t>(50);
    }
    
    file_internal void FreeRegistry(asset_registry *Registry)
    {
        for (u32 i = 0; i < Registry->Cap; ++i)
        {
            if (Registry->Assets[i])
            {
                FreeAsset(Registry->Assets[i]);
                Registry->Assets[i] = nullptr;
            }
        }
        
        pfree(Registry->Assets);
        AssetIdListFree(&Registry->ModelAssets);
        Registry->Count   = 0;
        Registry->Cap     = 0;
        Registry->NextIdx = 0;
        
        Registry->MaterialMap.Reset();
        Registry->TextureMap.Reset();
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
        
        asset_id_t Result = GenAssetId(Registry, Asset.Type);
        NewAsset->Id = Result;
        Registry->Assets[NewAsset->Id.Index] = NewAsset;
        Registry->Count++;
        
        return Result;
    }
    
    file_internal void RegistryRemove(asset_registry *Registry, u32 AssetIdx)
    {
        asset *Asset = Registry->Assets[AssetIdx];
        FreeAsset(Asset);
        ResourceAllocatorFree(&AssetAllocator, Asset);
        Registry->Assets[AssetIdx]->Id.Type = Asset_Invalid;
        Registry->Count--;
    }
    
    
    //~  Model Asset Functionality
    
    file_internal input_block_serial LoadInputBlock(FileBuffer *Buffer)
    {
        input_block_serial Result = {};
        
        ReadUInt32FromBinaryBuffer(Buffer, &Result.Size);
        ReadUInt32FromBinaryBuffer(Buffer, &Result.Set);
        ReadUInt32FromBinaryBuffer(Buffer, &Result.Binding);
        ReadBoolFromBinaryBuffer(Buffer, &Result.IsTextureBlock);
        ReadJStringFromBinaryBuffer(Buffer, &Result.Name);
        
        u32 MemberCount;
        ReadUInt32FromBinaryBuffer(Buffer, &MemberCount);
        Result.Members = DynamicArray<block_member_serial>(MemberCount);
        for (u32 Idx = 0; Idx < MemberCount; ++Idx)
        {
            block_member_serial Block = {};
            
            ReadUInt32FromBinaryBuffer(Buffer, &Block.Size);
            ReadUInt32FromBinaryBuffer(Buffer, &Block.Offset);
            ReadJStringFromBinaryBuffer(Buffer, &Block.Name);
            
            Result.Members.PushBack(Block);
        }
        
        return Result;
    }
    
    file_internal void LoadReflectionData(jstring ReflectionFile)
    {
        jstring Data = PlatformLoadFile(ReflectionFile);
        
        DynamicArray<shader_data_serial> ReflectionData = DynamicArray<shader_data_serial>(0);
        if (Data.len > 0)
        {
            FileBuffer Buffer = {};
            Buffer.start = Data.GetCStr();
            Buffer.brkp  = Buffer.start;
            Buffer.cap   = Data.len;
            
            u32 ShaderCount;
            ReadUInt32FromBinaryBuffer(&Buffer, &ShaderCount);
            ReflectionData.Resize(ShaderCount);
            for (u32 Idx = 0; Idx < ShaderCount; ++Idx)
            {
                shader_data_serial ShaderData = {};
                
                u32 ShaderType;
                ReadUInt32FromBinaryBuffer(&Buffer, &ShaderType);
                ShaderData.Type = static_cast<shader_type>(ShaderType);
                
                ReadUInt32FromBinaryBuffer(&Buffer, &ShaderData.DynamicSetSize);
                ReadUInt32FromBinaryBuffer(&Buffer, &ShaderData.StaticSetSize);
                ReadUInt32FromBinaryBuffer(&Buffer, &ShaderData.NumDynamicUniforms);
                ReadUInt32FromBinaryBuffer(&Buffer, &ShaderData.NumDynamicTextures);
                ReadUInt32FromBinaryBuffer(&Buffer, &ShaderData.NumStaticUniforms);
                ReadUInt32FromBinaryBuffer(&Buffer, &ShaderData.NumStaticTextures);
                
                ShaderData.PushConstants = LoadInputBlock(&Buffer);
                
                u32 DescriptorSetCount;
                ReadUInt32FromBinaryBuffer(&Buffer, &DescriptorSetCount);
                ShaderData.DescriptorSets = DynamicArray<input_block_serial>(DescriptorSetCount);
                for (u32 SetIdx = 0; SetIdx < DescriptorSetCount; ++SetIdx)
                {
                    input_block_serial Input = LoadInputBlock(&Buffer);
                    ShaderData.DescriptorSets.PushBack(Input);
                }
                
                u32 DynamicSetCount, GlobalSetCount, StaticSetCount;
                
                ReadUInt32FromBinaryBuffer(&Buffer, &DynamicSetCount);
                ShaderData.DynamicSets = DynamicArray<u32>(DynamicSetCount);
                for (u32 SetIdx = 0; SetIdx < DynamicSetCount; ++SetIdx)
                {
                    u32 Set;
                    ReadUInt32FromBinaryBuffer(&Buffer, &Set);
                    ShaderData.DynamicSets.PushBack(Set);
                }
                
                ReadUInt32FromBinaryBuffer(&Buffer, &GlobalSetCount);
                ShaderData.GlobalSets = DynamicArray<u32>(GlobalSetCount);
                for (u32 SetIdx = 0; SetIdx < GlobalSetCount; ++SetIdx)
                {
                    u32 Set;
                    ReadUInt32FromBinaryBuffer(&Buffer, &Set);
                    ShaderData.GlobalSets.PushBack(Set);
                }
                
                ReadUInt32FromBinaryBuffer(&Buffer, &StaticSetCount);
                ShaderData.StaticSets = DynamicArray<u32>(StaticSetCount);
                for (u32 SetIdx = 0; SetIdx < StaticSetCount; ++SetIdx)
                {
                    u32 Set;
                    ReadUInt32FromBinaryBuffer(&Buffer, &Set);
                    ShaderData.StaticSets.PushBack(Set);
                }
            }
        }
        
        // TODO(Dustin): Do something with the data
        /*

Need to collect each unique Descriptor Set

For each descriptor set, need to collect the Bindings that Create the Layout

After creating the Layout, we can create the descriptor sets

Need to make sure there are no gaps in the descriptor sets

*/
        
        struct mat_descriptor_set
        {
            u32 Set;
            
            // hard limit of 10 bindings
            VkDescriptorSetLayoutBinding Bindings[10];
            u32                          BindingsCount;
        };
        
        DynamicArray<mat_descriptor_set> SetList = DynamicArray<mat_descriptor_set>(5);
        
        // Step 1: Collect the unique descriptor sets.
        for (u32 Idx = 0; Idx < ReflectionData.size; ++Idx)
        {
            shader_data_serial ShaderData = ReflectionData[Idx];
            
            // TODO(Dustin): Push Constants...
            
            for (u32 SetIdx = 0; SetIdx < ShaderData.DescriptorSets.size; SetIdx++)
            {
                input_block_serial InputBlock = ShaderData.DescriptorSets[SetIdx];
                
                // Search for duplicate sets
                for (u32 SetListIdx = 0; SetListIdx < SetList.size; ++SetListIdx)
                {
                    if (SetList[SetListIdx].Set == InputBlock.Set)
                    {
                        
                        
                        // Add the binding...
                        // There are currently three types of bindings:
                        // 1. Uniform Block
                        // 2. Dynamic Uniform Block
                        // 3. Texture
                        // To distinguish 1 & 2:
                        // 1. "GlobalData" -> Use the default global data
                        // 2. "ObjectData" -> Use global object data
                        
                        // TODO(Dustin):
                        // For generic buffer types, I will assume that when i
                        // get to that point, I will be able to edit the data from
                        // a GUI.
                        
                        // TODO(Dustin):
                        // Textures are relatively simple:
                        // Check if the block is a texture, check if the texture is already
                        // created, then create the texture (if needed of course)
                        
                        if (InputBlock.Name == "ObjectData")
                        {
                            
                        }
                        else if (InputBlock.Name == "GlobalData")
                        {
                            
                        }
                        else if (InputBlock.IsTextureBlock)
                        {
                            
                        }
                        else
                        {
                            // TODO(Dustin):
                            // Generic descriptor bind, need to handle assigning data
                        }
                        
                        continue;
                    }
                }
            }
        }
        
        // Clean up
        Data.Clear();
    }
    
    struct texture
    {
        asset_id_t Id;
        
        VkFilter MagFilter;
        VkFilter MinFilter;
        
        VkSamplerAddressMode AddressModeU;
        VkSamplerAddressMode AddressModeV;
        VkSamplerAddressMode AddressModeW;
    };
    
    struct material_instance
    {
        bool HasPBRMetallicRoughness;
        bool HasPBRSpecularGlossiness;
        bool HasClearCoat;
        
        union {
            // Metallic - Roughness Pipeline
            struct
            {
                asset_id_t BaseColorTexture;
                asset_id_t MetallicRoughnessTexture;
                
                vec4           BaseColorFactor; // Will this always be a vec4?
                r32            MetallicFactor;
                r32            RoughnessFactor;
            };
            
            // Specilar - Glosiness Pipeline
            struct
            {
                asset_id_t DiffuseTexture;
                asset_id_t SpecularGlossinessTexture;
                
                vec4           DiffuseFactor;
                vec3           SpecularFactor;
                r32            GlossinessFactor;
            };
            
            // ClearCoat Pipeline
            struct
            {
                asset_id_t ClearCoatTexture;
                asset_id_t ClearCoatRoughnessTexture;
                asset_id_t ClearCoatNormalTexture;
                
                r32            ClearCoatFactor;
                r32            ClearCoatRoughnessFactor;
            };
        };
        
        asset_id_t NormalTexture;
        asset_id_t OcclusionTexture;
        asset_id_t EmissiveTexture;
        
        // Alpha mode?
        TextureAlphaMode AlphaMode;
        r32              AlphaCutoff;
        
        bool DoubleSided;
        bool Unlit;
    };
    
    // A material is defined by its name and the resources attached to it.
    struct material
    {
        jstring Name;
        
        shader_data       ShaderInfo;
        
        // while this data could be directly placed in the material struct
        // i want to prep for multiple material instances...having the below
        // instance struct, will allow for multiple instances to be attached
        // to the struct
        material_instance Instance;
    };
    
    file_internal asset_id_t LoadTexture(config_obj *TextureObj)
    {
        asset_id_t Result = {};
        
        jstring Filename = GetConfigStr(TextureObj, "Filename");
        
        // check to see if the texture_asset has already been loaded
        asset_id_t *Ret;
        if ((Ret = AssetRegistry.TextureMap.Get(Filename)))
        {
            mprint("Material \"%s\" is already loaded...Returing loaded asset.\n", Filename.GetCStr());
            Result = *Ret;
        }
        else
        {
            // Need to load the texture resources...
            
            // TODO(Dustin): Convert these to VK Objects
            i32 MagFilter = GetConfigI32(TextureObj, "MagFilter");
            i32 MinFilter = GetConfigI32(TextureObj, "MinFilter");
            
            i32 AddressModeU = GetConfigI32(TextureObj, "AddressModeU");
            i32 AddressModeV = GetConfigI32(TextureObj, "AddressModeV");
            i32 AddressModeW = GetConfigI32(TextureObj, "AddressModeW");
        }
        
        return Result;
    }
    
    file_internal asset_id_t LoadMaterial(jstring MaterialFilename, jstring MaterialName)
    {
        asset_id_t Result;
        
        asset_id_t *Ret;
        if ((Ret = AssetRegistry.MaterialMap.Get(MaterialName)))
        {
            mprint("Material \"%s\" is already loaded...Returing loaded asset.\n", MaterialName.GetCStr());
            Result = *Ret;
        }
        else
        {
            config_obj_table MaterialInfo = LoadConfigFile(MaterialFilename);
            
            config_obj MaterialSettings = GetConfigObj(&MaterialInfo, "Material");
            config_obj ShaderSettings   = GetConfigObj(&MaterialInfo, "Shaders");
            
            material Material = {};
            
            jstring ReflFile = GetConfigStr(&MaterialSettings, "ReflectionFile");
            LoadReflectionData(ReflFile);
            
            // TODO(Dustin): Handle other shader stages
            jstring VertFile = GetConfigStr(&ShaderSettings, "Vertex");
            jstring FragFile = GetConfigStr(&ShaderSettings, "Fragment");
            
            Material.Instance.HasPBRMetallicRoughness  = GetConfigI32(&MaterialSettings, "HasPBRMetallicRoughness");
            Material.Instance.HasPBRSpecularGlossiness = GetConfigI32(&MaterialSettings, "HasPBRSpecularGlossiness");
            Material.Instance.HasClearCoat             = GetConfigI32(&MaterialSettings, "HasClearCoat");
            Material.Instance.AlphaMode                = static_cast<TextureAlphaMode>(GetConfigI32(&MaterialSettings,
                                                                                                    "AlphaMode"));
            Material.Instance.AlphaCutoff              = GetConfigI32(&MaterialSettings, "AlphaCutoff");
            Material.Instance.DoubleSided              = GetConfigI32(&MaterialSettings, "DoubleSided");
            Material.Instance.Unlit                    = GetConfigI32(&MaterialSettings, "Unlit");
            
            if (Material.Instance.HasPBRMetallicRoughness)
            {
                config_obj MetallicSettings = GetConfigObj(&MaterialInfo, "Metallic-Roughness");
                
                Material.Instance.BaseColorFactor = GetConfigVec4(&MetallicSettings, "BaseColorFactor");
                Material.Instance.MetallicFactor  = GetConfigR32(&MetallicSettings, "MetallicFactor");
                Material.Instance.RoughnessFactor = GetConfigR32(&MetallicSettings, "RoughnessFactor");
                
                config_obj BaseColorTextureObj         = GetConfigObj(&MaterialInfo, "Base Color Texture");
                config_obj MetallicRoughnessTextureObj = GetConfigObj(&MaterialInfo, "Base Color Texture");
                
                Material.Instance.BaseColorTexture         = LoadTexture(&BaseColorTextureObj);
                Material.Instance.MetallicRoughnessTexture = LoadTexture(&MetallicRoughnessTextureObj);
            }
            
            if (Material.Instance.HasPBRSpecularGlossiness)
            {
                config_obj SpecularSettings = GetConfigObj(&MaterialInfo, "Specular-Glossy");
                
                Material.Instance.DiffuseFactor    = GetConfigVec4(&SpecularSettings, "DiffuseFactor");
                Material.Instance.SpecularFactor   = GetConfigVec3(&SpecularSettings, "SpecularFactor");
                Material.Instance.GlossinessFactor = GetConfigR32(&SpecularSettings, "GlossinessFactor");
                
                config_obj DiffuseTextureObj            = GetConfigObj(&MaterialInfo, "Diffuse Texture");
                config_obj SpecularGlossinessTextureObj = GetConfigObj(&MaterialInfo, "Specular Glossiness Texture");
                
                Material.Instance.DiffuseTexture            = LoadTexture(&DiffuseTextureObj);
                Material.Instance.SpecularGlossinessTexture = LoadTexture(&SpecularGlossinessTextureObj);
            }
            
            if (Material.Instance.HasClearCoat)
            {
                config_obj ClearCoatSettings = GetConfigObj(&MaterialInfo, "Clear Coat");
                
                Material.Instance.ClearCoatFactor          = GetConfigR32(&ClearCoatSettings, "ClearCoatFactor");
                Material.Instance.ClearCoatRoughnessFactor = GetConfigR32(&ClearCoatSettings, "ClearCoatRoughnessFactor");
                
                config_obj ClearCoatTextureObj          = GetConfigObj(&MaterialInfo, "Clear Coat Texture");
                config_obj ClearCoatRoughnessTextureObj = GetConfigObj(&MaterialInfo, "Clear Coat Roughness Texture");
                config_obj ClearCoatNormalTextureObj    = GetConfigObj(&MaterialInfo, "Clear Coat Normal Texture");
                
                Material.Instance.ClearCoatTexture          = LoadTexture(&ClearCoatTextureObj);
                Material.Instance.ClearCoatRoughnessTexture = LoadTexture(&ClearCoatRoughnessTextureObj);
                Material.Instance.ClearCoatNormalTexture    = LoadTexture(&ClearCoatNormalTextureObj);
            }
            
            config_obj NormalTextureObj    = GetConfigObj(&MaterialInfo, "Normal Texture");
            config_obj OcclusionTextureObj = GetConfigObj(&MaterialInfo, "Occlusion Texture");
            config_obj EmissiveTextureObj  = GetConfigObj(&MaterialInfo, "Emissive Texture");
            
            Material.Instance.NormalTexture    = LoadTexture(&NormalTextureObj);
            Material.Instance.OcclusionTexture = LoadTexture(&OcclusionTextureObj);
            Material.Instance.EmissiveTexture  = LoadTexture(&EmissiveTextureObj);
            
        }
        
        return Result;
    }
    
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
            
            jstring MaterialFilename;
            ReadJStringFromBinaryBuffer(&Loader->Buffer, &MaterialFilename);
            
            jstring MaterialName;
            ReadJStringFromBinaryBuffer(&Loader->Buffer, &MaterialName);
            
            asset_id_t MaterialId = LoadMaterial(MaterialFilename, MaterialName);
            
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
        asset_id_t Result;
        Result.Type = Asset_Invalid;
        
        asset Asset = {};
        
        switch (Type)
        {
            case Asset_Model:
            {
                model_create_info *Info = static_cast<model_create_info*>(Data);
                Asset = LoadModel(Info->FrameParams, Info->Filename);
                
                AssetIdListAdd(&AssetRegistry.ModelAssets, Asset.Id);
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
    
    void GetAssetList(asset **Assets, u32 *Count)
    {
        *Assets = talloc<asset>(AssetRegistry.Count);
        *Count  = AssetRegistry.Count;
        
        for (u32 Idx = 0; Idx < *Count; ++Idx) (*Assets)[Idx] = *AssetRegistry.Assets[Idx];
    }
    
    void FilterAssets(asset **Assets, u32 *Count, asset *AssetList, u32 AssetListCount, asset_type Type)
    {
        if (AssetList)
        {
            if (Type == Asset_Model)
            {
                *Assets = talloc<asset>(AssetRegistry.ModelAssets.Size);
                *Count  = AssetRegistry.ModelAssets.Size;
                
                for (u32 Idx = 0; Idx < *Count; ++Idx)
                {
                    u32 AssetIdx = AssetRegistry.ModelAssets.Ptr[Idx].Index;
                    (*Assets)[Idx] = *AssetRegistry.Assets[AssetIdx];
                }
            }
            else
            {
                // TODO(Dustin): Not implemnting yet, because do not need to.
            }
        }
        else
        {
            // TODO(Dustin): If an asset list is not provided, filter from the main Registry
            
        }
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
        FreeRegistry(&AssetRegistry);
        mm::FreeResourceAllocator(&AssetAllocator);
    }
    
    
    //~ Material Asset Functionality
    
    
    
    //~ Texture Asset Functionality
    
}; // masset