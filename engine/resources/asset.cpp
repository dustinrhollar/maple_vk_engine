
    inline bool operator==(const asset_id_t &Lhs, const asset_id_t &Rhs)
{
    return Lhs.Type == Rhs.Type && Lhs.Gen == Rhs.Gen && Lhs.Index == Rhs.Index;
}

inline bool operator!=(const asset_id_t &Lhs, const asset_id_t &Rhs)
{
    return Lhs.Type != Rhs.Type || Lhs.Gen != Rhs.Gen || Lhs.Index != Rhs.Index;
}

namespace masset
{
    //~ Special List for handling assets id lists
    
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
    file_internal asset_id_t LoadMaterial(mesh_loader *Loader, jstring MaterialFilename, jstring MaterialName);
    
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
        return Id.Type != Asset_Invalid &&
            Id.Index < Registry->Count &&
            Registry->Assets[Id.Index] &&
            Registry->Assets[Id.Index]->Id.Gen == Id.Gen;
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
        Result.Members = DynamicArray<block_member>(MemberCount);
        for (u32 Idx = 0; Idx < MemberCount; ++Idx)
        {
            block_member Block = {};
            
            ReadUInt32FromBinaryBuffer(Buffer, &Block.Size);
            ReadUInt32FromBinaryBuffer(Buffer, &Block.Offset);
            ReadJStringFromBinaryBuffer(Buffer, &Block.Name);
            
            Result.Members.PushBack(Block);
        }
        
        return Result;
    }
    
    file_internal shader_data LoadReflectionData(jstring ReflectionFile)
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
                
                ReflectionData.PushBack(ShaderData);
            }
        }
        
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
            
            resource_id_t DescriptorSet;
            resource_id_t DescriptorSetLayout;
        };
        
        // Find the maximum set number in the list. The sets were sorted when the reflection
        // data was generated so peaking into the last index for each shader_data should be
        // enough. Note, that there cannot be any gaps in descriptor set # when creating
        // a pipeline. So if any numbers are skipped, they will be filled in with an empty
        // descriptor set
        u32 MaxSetNumber = 0;
        for (u32 Idx = 0; Idx < ReflectionData.size; ++Idx)
        {
            shader_data_serial ShaderData = ReflectionData[Idx];
            
            // TODO(Dustin): Ok, for some reason the data is not sorted. Will
            // check out another time...
            for (u32 SetIdx = 0; SetIdx < ShaderData.DescriptorSets.size; ++SetIdx)
            {
                input_block_serial Block = ShaderData.DescriptorSets[SetIdx];
                
                if (Block.Set > MaxSetNumber) MaxSetNumber = Block.Set;
            }
        }
        
        // Initialize the list and initialize each descriptor
        // TODO(Dustin): Use talloc instead of a dynamic array since max size is known
        DynamicArray<mat_descriptor_set> SetList = DynamicArray<mat_descriptor_set>(MaxSetNumber + 1);
        for (u32 SetIdx = 0; SetIdx <= MaxSetNumber; SetIdx++)
        {
            mat_descriptor_set Set = {};
            Set.Set           = SetIdx;
            Set.BindingsCount = 0;
            
            SetList.PushBack(Set);
        }
        
        // Fill out the bindings
        // NOTE(Dustin): Currently, only the block named ObjectData will
        // be set to UNIFORM_DYNAMIC. In order to allow for dynamic uniforms
        // in the future, need to be able to set it through a GUI. However,
        // I am not yet quite sure how DYN Uniforms will be handled internally
        // for generic use cases.
        // All other buffers will be set to UNIFORM
        for (u32 Idx = 0; Idx < ReflectionData.size; ++Idx)
        {
            shader_data_serial ShaderData = ReflectionData[Idx];
            
            // TODO(Dustin): Push Constants...
            
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
            
            for (u32 SetIdx = 0; SetIdx < ShaderData.DescriptorSets.size; SetIdx++)
            {
                input_block_serial InputBlock = ShaderData.DescriptorSets[SetIdx];
                mat_descriptor_set *Set = &SetList[InputBlock.Set];
                
                
                VkDescriptorSetLayoutBinding Binding = {};
                Binding.binding            = InputBlock.Binding;
                Binding.descriptorCount    = 1;
                
                switch (ShaderData.Type)
                {
                    case Shader_Vertex:
                    {
                        Binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                    } break;
                    
                    case Shader_Fragment:
                    {
                        Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                    } break;
                    
                    case Shader_Geometry:
                    case Shader_TesselationControl:
                    case Shader_TesselationEvaluation:
                    case Shader_Compute:
                    default:
                    {
                        mprinte("Shader stage \"%d\" not supported!\n", ShaderData.Type);
                    } break;
                }
                
                Binding.pImmutableSamplers = nullptr; // Optional
                
                if (InputBlock.Name == "global_data_buffer")
                {
                    Binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                }
                else if (InputBlock.Name == "object_buffer")
                {
                    Binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                }
                else if (InputBlock.IsTextureBlock)
                {
                    // TODO(Dustin): Not handling texture right now...
                    // TODO(Dustin): Create/Attach the texture image?
                    Binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                }
                else
                {
                    // TODO(Dustin): Generic descriptor bind, need to handle assigning data
                    // NOTE(Dustin): For now, making it a Uniform buffer...
                    // TODO(Dustin): Create the buffer...?
                    Binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                }
                
                Set->Bindings[Set->BindingsCount++] = Binding;
            }
        }
        
        // Create the descriptor layouts + sets...
        // Bindings 0, 1 are hardcoded, and their layouts/descriptors can be obtained
        // from the Resource Manager. Any other descriptors are created.
        mresource::default_resources DefaultResources = mresource::GetDefaultResourcesFromRegistry();
        for (u32 SetIdx = 0; SetIdx < SetList.size; ++SetIdx)
        {
            // TODO(Dustin): For the hardcoded sets, make sure their
            // definitions match the expected definitions...
            if (SetList[SetIdx].Set == GLOBAL_SET)
            {
                SetList[SetIdx].DescriptorSet       = DefaultResources.DefaultGlobalDescriptor;
                SetList[SetIdx].DescriptorSetLayout = DefaultResources.DefaultGlobalDescriptorLayout;
            }
            else if (SetList[SetIdx].Set == STATIC_SET)
            {
                SetList[SetIdx].DescriptorSet       = DefaultResources.ObjectDescriptor;
                SetList[SetIdx].DescriptorSetLayout = DefaultResources.ObjectDescriptorLayout;
            }
            else
            {
                descriptor_layout_create_info LayoutCreateInfo = {};
                LayoutCreateInfo.BindingsCount = SetList[SetIdx].BindingsCount;
                LayoutCreateInfo.Bindings      = SetList[SetIdx].Bindings;
                SetList[SetIdx].DescriptorSetLayout = mresource::Load({}, // frame params aren't needed here
                                                                      Resource_DescriptorSetLayout,
                                                                      &LayoutCreateInfo);
                
                descriptor_create_info DescriptorCreateInfo = {};
                DescriptorCreateInfo.SetCount          = 1;
                DescriptorCreateInfo.DescriptorLayouts = &SetList[SetIdx].DescriptorSetLayout;
                SetList[SetIdx].DescriptorSet = mresource::Load({}, // frame params aren't needed here
                                                                Resource_DescriptorSet,
                                                                &DescriptorCreateInfo);
            }
        }
        
        // Convert set information to shader_data
        shader_data MaterialData = {};
        MaterialData.DescriptorSetsCount = SetList.size;
        MaterialData.DescriptorSets      = palloc<shader_descriptor_def>(MaterialData.DescriptorSetsCount);
        // NOTE(Dustin): Not going to initialize the known bind points...
        
        for (u32 SetIdx = 0; SetIdx < SetList.size; ++SetIdx)
        {
            mat_descriptor_set Set = SetList[SetIdx];
            
            MaterialData.DescriptorSets[SetIdx].DescriptorSet       = Set.DescriptorSet;
            MaterialData.DescriptorSets[SetIdx].DescriptorSetLayout = Set.DescriptorSetLayout;
        }
        
        // NOTE(Dustin): Not sure if this is necessary, but the intent is to determine the maximum
        // bind point so then exact amount of required memory is requested. Since the same set
        // can be used in multiple shader stages, it is important scan over the Reflection Array
        // and collect the max bind points across all shader stages
        
        // Determine the maximum set binding for each set.
        u32 *MaxBindPoints = talloc<u32>(MaterialData.DescriptorSetsCount);
        for (u32 BindPoint = 0; BindPoint < MaterialData.DescriptorSetsCount; ++BindPoint)
            MaxBindPoints[BindPoint] = 0;
        
        for (u32 ShaderBlock = 0; ShaderBlock < ReflectionData.size; ++ShaderBlock)
        {
            shader_data_serial ShaderData = ReflectionData[ShaderBlock];
            
            for (u32 SetIdx = 0; SetIdx < ShaderData.DescriptorSets.size; ++SetIdx)
            {
                input_block_serial InputBlock = ShaderData.DescriptorSets[SetIdx];
                
                if (MaxBindPoints[InputBlock.Set] < InputBlock.Binding)
                    MaxBindPoints[InputBlock.Set] = InputBlock.Binding;
            }
        }
        
        // Loop back through the descriptor sets and allocate the required memory
        for (u32 SetIdx = 0; SetIdx < MaterialData.DescriptorSetsCount; ++SetIdx)
        {
            MaterialData.DescriptorSets[SetIdx].InputDataCount = MaxBindPoints[SetIdx] + 1;
            MaterialData.DescriptorSets[SetIdx].InputData = palloc<input_block>(MaxBindPoints[SetIdx] + 1);
            
            for (u32 Binding = 0; Binding < MaterialData.DescriptorSets[SetIdx].InputDataCount; ++Binding)
                MaterialData.DescriptorSets[SetIdx].InputData[Binding] = {}; // Zero-Initialize the data
        }
        
        // Set the input block data for each descriptor
        for (u32 ShaderBlock = 0; ShaderBlock < ReflectionData.size; ++ShaderBlock)
        {
            // TODO(Dustin): Push constants....
            
            shader_data_serial ShaderData = ReflectionData[ShaderBlock];
            
            for (u32 SetIdx = 0; SetIdx < ShaderData.DescriptorSets.size; ++SetIdx)
            {
                input_block_serial SerialInputBlock = ShaderData.DescriptorSets[SetIdx];
                shader_descriptor_def *DescriptorDef = &MaterialData.DescriptorSets[SerialInputBlock.Set];
                
                u32 Binding = SerialInputBlock.Binding;
                input_block *InputBlock = &DescriptorDef->InputData[Binding];
                
                InputBlock->Name = CopyJString(SerialInputBlock.Name);
                InputBlock->IsTextureBlock = SerialInputBlock.IsTextureBlock;
                InputBlock->Size = SerialInputBlock.Size;
                
                InputBlock->MembersCount = SerialInputBlock.Members.size;
                InputBlock->Members = palloc<block_member>(InputBlock->MembersCount);
                
                for (u32 Member = 0; Member < InputBlock->MembersCount; ++Member)
                {
                    InputBlock->Members[Member] = SerialInputBlock.Members[Member];
                }
            }
        }
        
        // Clean up resources
        for (u32 ShaderBlock = 0; ShaderBlock < ReflectionData.size; ++ShaderBlock)
        {
            shader_data_serial *ShaderData = &ReflectionData[ShaderBlock];
            
            // Clear up push constant memory
            for (u32 Member = 0; Member < ShaderData->PushConstants.Members.size; ++Member)
                ShaderData->PushConstants.Members[Member].Name.Clear();
            
            ShaderData->PushConstants.Members.Reset();
            ShaderData->PushConstants.Name.Clear();
            
            // Clear up descriptors
            for (u32 SetIdx = 0; SetIdx < ShaderData->DescriptorSets.size; ++SetIdx)
            {
                for (u32 Member = 0; Member < ShaderData->DescriptorSets[SetIdx].Members.size; ++Member)
                    ShaderData->DescriptorSets[SetIdx].Members[Member].Name.Clear();
                
                ShaderData->DescriptorSets[SetIdx].Members.Reset();
                ShaderData->DescriptorSets[SetIdx].Name.Clear();
            }
            
            ShaderData->DescriptorSets.Reset();
            ShaderData->GlobalSets.Reset();
            ShaderData->StaticSets.Reset();
            ShaderData->DynamicSets.Reset();
        }
        
        SetList.Reset();
        Data.Clear();
        
        return MaterialData;
    }
    
    // Use for getting the wrap_t and wrap_s
    inline VkSamplerAddressMode Int32ToVkAddressMode(i32 address_mode)
    {
        switch (address_mode)
        {
            // wrap_s and wrap_t
            case 33071: // CLAMP_TO_EDGE
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            
            case 33648: // MIRRORED_REPEAT
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            
            case 10497: // REPEAT
            default:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    }
    
    // used for getting the mag and min filter
    inline VkFilter Int32ToVkFilter(i32 filter)
    {
        switch (filter)
        {
            // Mag + Min Filter
            case 9728: // NEAREST
            return VK_FILTER_NEAREST;
            
            case 9729: // LINEAR
            return VK_FILTER_LINEAR;
            
            // Min Filter
            // By default, these values are not within the vulkan specs
            case 9984: // NEAREST_MIPMAP_NEAREST
            case 9985: // LINEAR_MIPMAP_NEAREST
            case 9986: // NEAREST_MIPMAP_LINEAR
            case 9987: // LINEAR_MIPMAP_LINEAR
            
            default: return VK_FILTER_NEAREST;
        }
    }
    
    file_internal asset_id_t LoadTexture(mesh_loader *Loader, config_obj *TextureObj)
    {
        asset_id_t Result = {};
        
        jstring Filename = GetConfigStr(TextureObj, "Filename");
        
        // check to see if the texture_asset has already been loaded
        asset_id_t *Ret;
        if ((Ret = AssetRegistry.TextureMap.Get(Filename)))
        {
            mprint("Texture \"%s\" is already loaded...Returing loaded asset.\n", Filename.GetCStr());
            Result = *Ret;
        }
        else
        {
            jstring Filename = GetConfigStr(TextureObj, "Filename");
            if (Filename.len > 0)
            {
                i32 MagFilter = GetConfigI32(TextureObj, "MagFilter");
                i32 MinFilter = GetConfigI32(TextureObj, "MinFilter");
                
                i32 AddressModeU = GetConfigI32(TextureObj, "AddressModeU");
                i32 AddressModeV = GetConfigI32(TextureObj, "AddressModeV");
                i32 AddressModeW = GetConfigI32(TextureObj, "AddressModeW");
                
                image_create_info *TextureInfo = talloc<image_create_info>();
                TextureInfo->Filename     = CopyJString(Filename);
                TextureInfo->MagFilter    = Int32ToVkFilter(MagFilter);
                TextureInfo->MinFilter    = Int32ToVkFilter(MinFilter);
                TextureInfo->AddressModeU = Int32ToVkAddressMode(AddressModeU);
                TextureInfo->AddressModeV = Int32ToVkAddressMode(AddressModeV);
                TextureInfo->AddressModeW = Int32ToVkAddressMode(AddressModeW);
                
                asset_texture Texture = {};
                Texture.Image = mresource::Load(Loader->FrameParams, Resource_Image, TextureInfo);
                
                asset Asset = {};
                Asset.Type    = Asset_Texture;
                Asset.Texture = Texture;
                
                Result = RegistryAdd(&AssetRegistry, Asset);
                AssetRegistry.TextureMap.Insert(Filename, Result);
                
                Filename.Clear();
            }
            else
            {
                Result = INVALID_ASSET;
            }
        }
        
        return Result;
    }
    
    file_internal asset_id_t LoadMaterial(mesh_loader *Loader, jstring MaterialFilename, jstring MaterialName)
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
            
            asset_material Material = {};
            Material.Name = CopyJString(GetConfigStr(&MaterialSettings, "Name"));
            
            //~ Load Reflection Info and create descriptors
            
            jstring ReflFile = GetConfigStr(&MaterialSettings, "ReflectionFile");
            Material.ShaderData = LoadReflectionData(ReflFile);
            
            //~ Load Material Instance Data
            
            Material.Instance.HasPBRMetallicRoughness  = GetConfigI32(&MaterialSettings, "HasPBRMetallicRoughness");
            Material.Instance.HasPBRSpecularGlossiness = GetConfigI32(&MaterialSettings, "HasPBRSpecularGlossiness");
            Material.Instance.HasClearCoat             = GetConfigI32(&MaterialSettings, "HasClearCoat");
            Material.Instance.AlphaMode                = static_cast<TextureAlphaMode>(GetConfigI32(&MaterialSettings,
                                                                                                    "AlphaMode"));
            Material.Instance.AlphaCutoff              = GetConfigR32(&MaterialSettings, "AlphaCutoff");
            Material.Instance.DoubleSided              = GetConfigI32(&MaterialSettings, "DoubleSided");
            Material.Instance.Unlit                    = GetConfigI32(&MaterialSettings, "Unlit");
            
            if (Material.Instance.HasPBRMetallicRoughness)
            {
                config_obj MetallicSettings = GetConfigObj(&MaterialInfo, "Metallic-Roughness");
                
                Material.Instance.BaseColorFactor = GetConfigVec4(&MetallicSettings, "BaseColorFactor");
                Material.Instance.MetallicFactor  = GetConfigR32(&MetallicSettings, "MetallicFactor");
                Material.Instance.RoughnessFactor = GetConfigR32(&MetallicSettings, "RoughnessFactor");
                
                config_obj BaseColorTextureObj         = GetConfigObj(&MaterialInfo, "Base Color Texture");
                config_obj MetallicRoughnessTextureObj = GetConfigObj(&MaterialInfo, "Metallic Roughness Texture");
                
                Material.Instance.BaseColorTexture         = LoadTexture(Loader, &BaseColorTextureObj);
                Material.Instance.MetallicRoughnessTexture = LoadTexture(Loader, &MetallicRoughnessTextureObj);
            }
            
            if (Material.Instance.HasPBRSpecularGlossiness)
            {
                config_obj SpecularSettings = GetConfigObj(&MaterialInfo, "Specular-Glossy");
                
                Material.Instance.DiffuseFactor    = GetConfigVec4(&SpecularSettings, "DiffuseFactor");
                Material.Instance.SpecularFactor   = GetConfigVec3(&SpecularSettings, "SpecularFactor");
                Material.Instance.GlossinessFactor = GetConfigR32(&SpecularSettings, "GlossinessFactor");
                
                config_obj DiffuseTextureObj            = GetConfigObj(&MaterialInfo, "Diffuse Texture");
                config_obj SpecularGlossinessTextureObj = GetConfigObj(&MaterialInfo, "Specular Glossiness Texture");
                
                Material.Instance.DiffuseTexture            = LoadTexture(Loader, &DiffuseTextureObj);
                Material.Instance.SpecularGlossinessTexture = LoadTexture(Loader, &SpecularGlossinessTextureObj);
            }
            
            if (Material.Instance.HasClearCoat)
            {
                config_obj ClearCoatSettings = GetConfigObj(&MaterialInfo, "Clear Coat");
                
                Material.Instance.ClearCoatFactor          = GetConfigR32(&ClearCoatSettings, "ClearCoatFactor");
                Material.Instance.ClearCoatRoughnessFactor = GetConfigR32(&ClearCoatSettings, "ClearCoatRoughnessFactor");
                
                config_obj ClearCoatTextureObj          = GetConfigObj(&MaterialInfo, "Clear Coat Texture");
                config_obj ClearCoatRoughnessTextureObj = GetConfigObj(&MaterialInfo, "Clear Coat Roughness Texture");
                config_obj ClearCoatNormalTextureObj    = GetConfigObj(&MaterialInfo, "Clear Coat Normal Texture");
                
                Material.Instance.ClearCoatTexture          = LoadTexture(Loader, &ClearCoatTextureObj);
                Material.Instance.ClearCoatRoughnessTexture = LoadTexture(Loader, &ClearCoatRoughnessTextureObj);
                Material.Instance.ClearCoatNormalTexture    = LoadTexture(Loader, &ClearCoatNormalTextureObj);
            }
            
            config_obj NormalTextureObj    = GetConfigObj(&MaterialInfo, "Normal Texture");
            config_obj OcclusionTextureObj = GetConfigObj(&MaterialInfo, "Occlusion Texture");
            config_obj EmissiveTextureObj  = GetConfigObj(&MaterialInfo, "Emissive Texture");
            
            Material.Instance.NormalTexture    = LoadTexture(Loader, &NormalTextureObj);
            Material.Instance.OcclusionTexture = LoadTexture(Loader, &OcclusionTextureObj);
            Material.Instance.EmissiveTexture  = LoadTexture(Loader, &EmissiveTextureObj);
            
            //~ Now that textures have been loaded, let's bind the textures to the descriptors...
            
            if (Material.ShaderData.DescriptorSetsCount > DYNAMIC_SET)
            {
                shader_descriptor_def MaterialSet = Material.ShaderData.DescriptorSets[DYNAMIC_SET];
                
                descriptor_write_info *WriteInfos = talloc<descriptor_write_info>(MaterialSet.InputDataCount);
                u32 WriteInfoCount = 0;
                
                
                if (Material.Instance.HasPBRMetallicRoughness)
                {
                    if (RegistryIsValidAsset(&AssetRegistry,
                                             Material.Instance.BaseColorTexture))
                    {
                        asset *Asset = AssetRegistry.Assets[Material.Instance.BaseColorTexture.Index];
                        
                        descriptor_write_info Info = {
                            Asset->Texture.Image,
                            MaterialSet.DescriptorSet,
                            BASE_COLOR_TEXTURE_BINDING,
                            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                        };
                        
                        WriteInfos[WriteInfoCount++] = Info;
                    }
                    
                    if (RegistryIsValidAsset(&AssetRegistry,
                                             Material.Instance.MetallicRoughnessTexture))
                    {
                        asset *Asset = AssetRegistry.Assets[Material.Instance.MetallicRoughnessTexture.Index];
                        
                        descriptor_write_info Info = {
                            Asset->Texture.Image,
                            MaterialSet.DescriptorSet,
                            METALLIC_ROUGHNESS_TEXTURE_BINDING,
                            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                        };
                        
                        WriteInfos[WriteInfoCount++] = Info;
                    }
                }
                
                // TODO(Dustin): Specular-Glossiness and Clearcut workflow
                
                
                if (RegistryIsValidAsset(&AssetRegistry,
                                         Material.Instance.NormalTexture))
                {
                    asset *Asset = AssetRegistry.Assets[Material.Instance.NormalTexture.Index];
                    
                    descriptor_write_info Info = {
                        Asset->Texture.Image,
                        MaterialSet.DescriptorSet,
                        NORMAL_TEXTURE_BINDING,
                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                    };
                    
                    WriteInfos[WriteInfoCount++] = Info;
                }
                
                
                if (RegistryIsValidAsset(&AssetRegistry,
                                         Material.Instance.OcclusionTexture))
                {
                    asset *Asset = AssetRegistry.Assets[Material.Instance.OcclusionTexture.Index];
                    
                    descriptor_write_info Info = {
                        Asset->Texture.Image,
                        MaterialSet.DescriptorSet,
                        OCCLUSION_TEXTURE_BINDING,
                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                    };
                    
                    WriteInfos[WriteInfoCount++] = Info;
                }
                
                if (RegistryIsValidAsset(&AssetRegistry,
                                         Material.Instance.EmissiveTexture))
                {
                    asset *Asset = AssetRegistry.Assets[Material.Instance.EmissiveTexture.Index];
                    
                    descriptor_write_info Info = {
                        Asset->Texture.Image,
                        MaterialSet.DescriptorSet,
                        EMISSIVE_TEXTURE_BINDING,
                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                    };
                    
                    WriteInfos[WriteInfoCount++] = Info;
                }
                
                gpu_descriptor_update_info *DescriptorUpdateInfos = talloc<gpu_descriptor_update_info>();
                DescriptorUpdateInfos->WriteInfos      = WriteInfos;
                DescriptorUpdateInfos->WriteInfosCount = WriteInfoCount;
                
                AddGpuCommand(Loader->FrameParams, {GpuCmd_UpdateDescriptor, DescriptorUpdateInfos});
            }
            
            //~ Create the Shader Pipeline
            
            // TODO(Dustin): Handle other shader stages
            jstring VertFile = GetConfigStr(&ShaderSettings, "Vertex");
            jstring FragFile = GetConfigStr(&ShaderSettings, "Fragment");
            
            
            shader_file_create_info Shaders[] = {
                { VertFile, VK_SHADER_STAGE_VERTEX_BIT   },
                { FragFile, VK_SHADER_STAGE_FRAGMENT_BIT }
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
            
            resource_id_t *Layouts = talloc<resource_id_t>(Material.ShaderData.DescriptorSetsCount);
            for (u32 SetIdx = 0; SetIdx < Material.ShaderData.DescriptorSetsCount; ++SetIdx)
                Layouts[SetIdx] = Material.ShaderData.DescriptorSets[SetIdx].DescriptorSetLayout;
            
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
            PipelineCreateInfo.DescriptorLayoutsCount = Material.ShaderData.DescriptorSetsCount;
            PipelineCreateInfo.PushConstants          = nullptr;
            PipelineCreateInfo.PushConstantsCount     = 0;
            PipelineCreateInfo.RenderPass             = GetPrimaryRenderPass();
            
            Material.Pipeline = mresource::Load({},
                                                Resource_Pipeline,
                                                &PipelineCreateInfo);
            
            asset Asset = {};
            Asset.Type     = Asset_Material;
            Asset.Material = Material;
            
            Result = RegistryAdd(&AssetRegistry, Asset);
            AssetRegistry.MaterialMap.Insert(Material.Name, Result);
            
            //~ clean memory
            
            FreeConfigObjTable(&MaterialInfo);
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
            
            Primitive.Material = LoadMaterial(Loader, MaterialFilename, MaterialName);
            
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
                
                Result = RegistryAdd(&AssetRegistry, Asset);
                AssetIdListAdd(&AssetRegistry.ModelAssets, Result);
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
        
        return Result;
    }
    
    asset* GetAsset(asset_id_t Id)
    {
        return AssetRegistry.Assets[Id.Index];
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
                // NOTE(Dustin): Yo, this isn't using the passed AssetList!
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
            switch (Type)
            {
                case Asset_Model:
                {
                    *Assets = talloc<asset>(AssetRegistry.ModelAssets.Size);
                    *Count  = AssetRegistry.ModelAssets.Size;
                    
                    for (u32 Idx = 0; Idx < *Count; ++Idx)
                    {
                        u32 AssetIdx = AssetRegistry.ModelAssets.Ptr[Idx].Index;
                        (*Assets)[Idx] = *AssetRegistry.Assets[AssetIdx];
                    }
                    
                } break;
                
                case Asset_Material:
                {
                    *Count = AssetRegistry.MaterialMap.Count;
                    *Assets = talloc<asset>(*Count);
                    
                    u32 MatIdx = 0;
                    for (u32 AssetIdx = 0; AssetIdx < AssetRegistry.Count; ++AssetIdx)
                    {
                        asset *Asset = AssetRegistry.Assets[AssetIdx];
                        if (Asset->Type == Asset_Material) (*Assets)[MatIdx++] = *Asset;
                    }
                    
                } break;
                
                default: break;
            }
            
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
    
    asset_registry* GetAssetRegistry()
    {
        return &AssetRegistry;
    }
    
}; // masset