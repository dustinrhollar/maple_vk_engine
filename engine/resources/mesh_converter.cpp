
namespace masset
{
    const char *MODEL_OUTPUT_FOLDER      = "data/models/models/";
    const char *BINARY_OUTPUT_FOLDER     = "data/models/binaries/";
    const char *MATERIAL_OUTPUT_FOLDER   = "data/materials/";
    const char *TEXTURE_OUTPUT_FOLDER    = "data/textures/";
    const char *REFLECTION_OUTPUT_FOLDER = "data/mat_refl/";
    const char *BINARY_EXTENSION         = ".bin";
    const char *MODEL_EXTENSION          = ".model";
    
    file_internal i32 ConvertNode(mesh_converter *Converter, cgltf_node *CgNode);
    file_internal i32 ConvertScene(mesh_converter *Converter, cgltf_scene *CgScene);
    
    file_internal i32 ConvertPrimitive(mesh_converter *Converter, cgltf_primitive *CgPrimitive)
    {
        primitive_serial Primitive = {};
        Primitive.PrimitiveIdx = Converter->PrimitiveIdx++;
        Primitive.VertexStride = sizeof(Vertex);
        Primitive.IndexStride  = sizeof(u32);
        Primitive.MaterialFile = InitJString();
        
        if (CgPrimitive->indices)
        {
            Primitive.IndexCount = CgPrimitive->indices->count;
            Primitive.IsIndexed  = true;
        }
        else
        {
            Primitive.IndexCount  = 0;
            Primitive.IsIndexed   = false;
        }
        
        if (CgPrimitive->material)
        {
            Primitive.MaterialName = InitJString(CgPrimitive->material->name);
            jstring *Filename = Converter->MaterialFilenameMap.Get(Primitive.MaterialName);
            
            if (Filename)
            {
                Primitive.MaterialFile = *Filename;
            }
            else
                mprinte("Could not find a material with the name \"%s\"!\n", Primitive.MaterialName.GetCStr());
        }
        else
        {
            mprinte("Primitive does not have a material!\n");
        }
        
        // figure out the vertex count
        for (size_t k = 0; k < CgPrimitive->attributes_count; ++k)
        { // find the position attribute
            cgltf_attribute attrib = CgPrimitive->attributes[k];
            
            if (attrib.type == cgltf_attribute_type_position)
            {
                cgltf_accessor *accessor = attrib.data;
                
                Primitive.VertexCount = attrib.data->count;
                
                // TODO(Dustin): If the Min/Max is not provided need to manually
                // track the min/max points while copying over the vertex data.
                if (accessor->has_min)
                {
                    Primitive.Min = MakeVec3(accessor->min);
                }
                
                if (accessor->has_max)
                {
                    Primitive.Max = MakeVec3(accessor->max);
                }
                
                break;
            }
        }
        
        size_t DataSize = sizeof(u32) * Primitive.IndexCount + sizeof(Vertex) * Primitive.VertexCount;
        ResizeIfPossible(&Converter->PrimitiveDataBlock, DataSize);
        
        Primitive.Offset = Converter->PrimitiveDataBlock.brkp - Converter->PrimitiveDataBlock.start;
        
        u32 IndicesOffset  = Primitive.Offset;
        u32 VerticesOffset = IndicesOffset + sizeof(u32) * Primitive.IndexCount;
        
        Converter->PrimitiveDataBlock.brkp += VerticesOffset + sizeof(Vertex) * Primitive.VertexCount;
        
        bool HasPosition = false;
        bool HasNormals  = false;
        //bool HasColor    = false;
        //bool HasUVs      = false;
        //bool HasWeights  = false; 
        //bool HasJoints   = false;
        
        float *position_buffer = nullptr;
        float *normal_buffer   = nullptr;
        float *color_buffer    = nullptr;
        float *uv0_buffer      = nullptr;
        //float *weights_buffer  = nullptr;
        //float *joints_buffer   = nullptr;
        
        // Determine the attributes and get the ptr to their buffers
        for (size_t k = 0; k < CgPrimitive->attributes_count; ++k)
        { // find the position attribute
            cgltf_attribute attrib = CgPrimitive->attributes[k];
            
            cgltf_accessor *accessor = attrib.data;
            cgltf_buffer_view *buffer_view = accessor->buffer_view;
            
            // NOTE(Dustin): Might have to use top level buffer rather than buffer view
            char *buffer = (char*)buffer_view->buffer->data;
            
            switch (attrib.type)
            {
                case cgltf_attribute_type_position:
                {
                    HasPosition = true;
                    position_buffer = (float*)(buffer + accessor->offset + buffer_view->offset);
#if 0
                    printf("Copied Positions:\n----------------------------------------\n");
                    float *iter = position_buffer;
                    for (int i = 0; i < Primitive.VertexCount; ++i)
                    {
                        printf("Vertex %d\n", i);
                        printf("\tPosition %lf %lf %lf\n", (*iter), (*iter), (*iter));
                        iter += 3;
                    }
                    printf("----------------------------------------\n");
#endif
                } break;
                
                case cgltf_attribute_type_normal:
                {
                    HasNormals = true;
                    normal_buffer = (float*)(buffer + accessor->offset + buffer_view->offset);
                    
#if 0
                    printf("Copied Normals:\n----------------------------------------\n");
                    float *iter = normal_buffer;
                    for (int i = 0; i < Primitive.VertexCount; ++i)
                    {
                        printf("Vertex %d\n", i);
                        printf("\tNormals  %lf %lf %lf\n", (*iter), (*iter), (*iter));
                        
                        iter += 3;
                    }
                    printf("----------------------------------------\n");
#endif
                } break;
                
                case cgltf_attribute_type_color:
                {
                    //HasColor = true;
                    color_buffer = (float*)(buffer + accessor->offset + buffer_view->offset);
                } break;
                
                case cgltf_attribute_type_texcoord:
                {
                    //HasUVs = true;
                    uv0_buffer = (float*)(buffer + accessor->offset + buffer_view->offset);
                } break;
                
                case cgltf_attribute_type_weights:
                {
                    //HasUVs = true;
                    //weights_buffer = (float*)(buffer + accessor->offset + buffer_view->offset);
                } break;
                
                case cgltf_attribute_type_joints:
                {
                    //HasUVs = true;
                    //joints_buffer = (float*)(buffer + accessor->offset + buffer_view->offset);
                } break;
                
                // TODO(Dustin): tangents,
                default: break;
            }
        }
        
        if (!HasPosition || !HasNormals)
        { /* TODO(Dustin): LOG */ }
        
        // Retrieve the index buffer data
        u32 *indices = nullptr;
        if (Primitive.IndexCount > 0)
        {
            indices = (u32*)((char*)Converter->PrimitiveDataBlock.start + IndicesOffset);
            cgltf_accessor *indices_accessor = CgPrimitive->indices;
            cgltf_buffer_view *indices_buffer_view = indices_accessor->buffer_view;
            
            // NOTE(Dustin): Might have to use top level buffer rather than buffer view
            char *unknown_indices_buffer = (char*)indices_buffer_view->buffer->data;
            
            // Copy the indices over
            // NOTE(Dustin): It is possible that the index type is not u16, so need
            // to verify its type
            switch (indices_accessor->component_type)
            {
                case cgltf_component_type_r_16u:
                {
                    u16 *indices_buffer = (u16*)(unknown_indices_buffer +
                                                 //indices_accessor->offset +
                                                 indices_buffer_view->offset);
                    for (u32 i = 0; i < Primitive.IndexCount; ++i)
                    {
                        indices[i] = (u32)indices_buffer[i];
                    }
                } break;
                
                case cgltf_component_type_r_8u:
                {
                    u8 *indices_buffer = (u8*)unknown_indices_buffer;
                    
                    for (u32 i = 0; i < Primitive.IndexCount; ++i)
                    {
                        indices[i] = (u32)indices_buffer[i];
                    }
                } break;
                
                case cgltf_component_type_r_32u:
                {
                    u32 *indices_buffer = (u32*)unknown_indices_buffer;
                    
                    for (u32 i = 0; i < Primitive.IndexCount; ++i)
                    {
                        indices[i] = (u32)indices_buffer[i];
                    }
                } break;
                
                default:
                { /* TODO(Dustin): Might want to Log the invalid index size */ } break;
            }
            
#if 0
            // TEST CODE
            u16 *iter = (u16*)((char*)Primitive.DataBlock + Primitive.IndicesOffset);
            for (int i = 0; i < Primitive.IndexCount; i += 3)
            {
                printf("%d %d %d\n", *(iter+0), *(iter+1), *(iter+3));
                
                iter += 3;
            }
            printf("----------------------------------------\n");
#endif
        }
        
        
        // Copy the vertex data over to its memory block
        {
            Vertex *vertices = (Vertex*)((char*)Converter->PrimitiveDataBlock.start + VerticesOffset);
            
            vec2 dummy_vec2 = { 0.0f, 0.0f };
            vec3 dummy_vec3 = { 0.f, 0.0f, 0.0f };
            vec4 dummy_vec4 = { 0.5f, 0.5f, 0.5f, 1.0f };
            
            for (size_t idx = 0; idx < Primitive.VertexCount; ++idx)
            {
                vertices[idx].Position = MakeVec3(position_buffer + idx * 3);
                vertices[idx].Normals  = (normal_buffer) ? MakeVec3(normal_buffer + idx * 3) : dummy_vec3;
                vertices[idx].Color    = (color_buffer)  ? MakeVec4(color_buffer + idx * 4)  : dummy_vec4;
                vertices[idx].Tex0     = (uv0_buffer)    ? MakeVec2(uv0_buffer + idx * 2)    : dummy_vec2;
                
#if 0
                printf("Vertex %d\n", i);
                printf("\tPosition %lf %lf %lf\n", vertices[idx].Position.x, vertices[idx].Position.y, vertices[idx].Position.z);
                printf("\tNormals  %lf %lf %lf\n", vertices[idx].Normals.x, vertices[idx].Normals.y, vertices[idx].Normals.z);
                printf("\tColor    %lf %lf %lf %lf\n", vertices[idx].Color.x, vertices[idx].Color.y, vertices[idx].Color.z, vertices[i].Color.w);
                printf("\tTex0     %lf %lf\n", vertices[idx].Tex0.x, vertices[idx].Tex0.y);
#endif
            }
        }
        
        Converter->SerialPrimitiveList[Primitive.PrimitiveIdx] = Primitive;
        
        return Primitive.PrimitiveIdx;
    }
    
    file_internal i32 ConvertMesh(mesh_converter *Converter, cgltf_mesh *CgMesh)
    {
        mesh_serial Mesh     = {};
        Mesh.MeshIdx         = Converter->MeshIdx++;
        Mesh.PrimitivesCount = CgMesh->primitives_count;
        Mesh.PrimitivesIdx   = talloc<i32>(Mesh.PrimitivesCount);
        
        for (i32 i = 0; i < (i32)CgMesh->primitives_count; ++i)
        {
            i32 PrimitiveIdx = ConvertPrimitive(Converter, CgMesh->primitives + i);
            if (PrimitiveIdx >= 0) Mesh.PrimitivesIdx[i] = PrimitiveIdx;
        }
        
        Converter->SerialMeshList[Mesh.MeshIdx] = Mesh;
        
        return Mesh.MeshIdx;
    }
    
    file_internal i32 ConvertNode(mesh_converter *Converter, cgltf_node *CgNode)
    {
        model_node_serial Node = {};
        Node.NodeIdx   = Converter->NodeIdx++;
        Node.Name      = InitJString();
        Node.MeshName  = InitJString();
        Node.MeshIdx   = -1;
        Node.ParentIdx = -1;
        
        if (CgNode->name)
        {
            Node.Name = pstring(CgNode->name);
        }
        
        if (CgNode->has_translation)
        {
            Node.HasTranslation = true;
            Node.Translation = MakeVec3(CgNode->translation);
        }
        else
        {
            Node.HasTranslation = false;
            Node.Translation = {0.0f,0.0f,0.0f};
        }
        
        if (CgNode->has_scale)
        {
            Node.HasScale = true;
            Node.Scale = MakeVec3(CgNode->scale);
        }
        else
        {
            Node.HasScale = false;
            Node.Scale = {1.0f,1.0f,1.0f};
        }
        
        if (CgNode->has_rotation)
        {
            Node.HasRotation = true;
            Node.Rotation = MakeVec4(CgNode->rotation);
        }
        else
        {
            Node.HasRotation = false;
            Node.Rotation = {0.0f,0.0f,0.0f,1.0f};
        }
        
        if (CgNode->has_matrix)
        {
            Node.HasMatrix = true;
            Node.Matrix = MakeMat4(CgNode->matrix);
        }
        else
        {
            Node.HasMatrix = false;
            Node.Matrix = mat4(1.0f);
        }
        
        if (CgNode->mesh)
        {
            // TODO(Dustin): Assign node as mesh node type
            i32 MeshIdx = ConvertMesh(Converter, CgNode->mesh);
            Node.MeshIdx = MeshIdx;
        }
        else
        {
            // TODO(Dustin): Lookup into a node reference array to determine if the node
            // is a Joint node type and if the joint is a root joint
        }
        
        // Recurse down the set to parse all child nodes
        Node.ChildrenCount   = CgNode->children_count;
        Node.ChildrenIndices = talloc<i32>(Node.ChildrenCount);
        
        for (u64 i = 0; i < Node.ChildrenCount; ++i)
        {
            i32 ChildIdx = ConvertNode(Converter, CgNode->children[i]);
            Node.ChildrenIndices[i] = ChildIdx;
        }
        
        Converter->SerialNodeList[Node.NodeIdx] = Node;
        for (u64 i = 0; i < Node.ChildrenCount; ++i)
        {
            i32 ChildIdx = Node.ChildrenIndices[i];
            Converter->SerialNodeList[ChildIdx].ParentIdx = Node.NodeIdx;
        }
        
        return Node.NodeIdx;
    }
    
    file_internal i32 ConvertScene(mesh_converter *Converter, cgltf_scene *CgScene)
    {
        scene_serial Scene = {};
        Scene.SceneIdx     = Converter->SceneIdx++;
        Scene.NodesCount   = (u32)CgScene->nodes_count;
        Scene.NodesIdx     = talloc<i32>(Scene.NodesCount);
        
        for (i32 i = 0; i < (i32)Scene.NodesCount; ++i)
        {
            i32 Idx = ConvertNode(Converter, CgScene->nodes[i]);
            Scene.NodesIdx[i] = Idx;
        }
        
        Converter->SerialSceneList[Scene.SceneIdx] = Scene;
        
        return Scene.SceneIdx;
    }
    
    file_internal bool CheckTextBufferResize(FileBuffer *Buffer, i32 WrittenChars)
    {
        bool Result = false;
        if (WrittenChars > BufferUnusedSize(Buffer))
        {
            ResizeFileBuffer(Buffer, Buffer->cap * 2);
            Result = true;
        }
        return Result;
    }
    
    file_internal void SerializeTexture(FileBuffer *Buffer, texture_serial *Texture, const char *TextureHeader)
    {
        i32 Ret;
        
        Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "{%s}\n", TextureHeader);
        if (CheckTextBufferResize(Buffer, Ret))
            Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "{%s}\n", TextureHeader);
        Buffer->brkp += Ret;
        
        Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "Filename : str = \"%s\"\n",
                                Texture->Filename.GetCStr());
        if (CheckTextBufferResize(Buffer, Ret))
            Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "Filename : str = \"%s\"\n",
                                    Texture->Filename.GetCStr());
        Buffer->brkp += Ret;
        
        Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "MinFilter : i32 = %d\n",
                                Texture->MinFilter);
        if (CheckTextBufferResize(Buffer, Ret))
            Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "MinFilter : i32 = %d\n",
                                    Texture->MinFilter);
        Buffer->brkp += Ret;
        
        Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "MagFilter : i32 = %d\n",
                                Texture->MagFilter);
        if (CheckTextBufferResize(Buffer, Ret))
            Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "MagFilter : i32 = %d\n",
                                    Texture->MagFilter);
        Buffer->brkp += Ret;
        
        Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "AddressModeU : i32 = %d\n",
                                Texture->AddressModeU);
        if (CheckTextBufferResize(Buffer, Ret))
            Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "AddressModeU : i32 = %d\n",
                                    Texture->AddressModeU);
        Buffer->brkp += Ret;
        
        Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "AddressModeV : i32 = %d\n",
                                Texture->AddressModeV);
        if (CheckTextBufferResize(Buffer, Ret))
            Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "AddressModeV : i32 = %d\n",
                                    Texture->AddressModeV);
        Buffer->brkp += Ret;
        
        Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "AddressModeW : i32 = %d\n",
                                Texture->AddressModeW);
        if (CheckTextBufferResize(Buffer, Ret))
            Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "AddressModeW : i32 = %d\n",
                                    Texture->AddressModeW);
        Buffer->brkp += Ret;
        
        { // spacing...
            Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "\n");
            if (CheckTextBufferResize(Buffer, Ret))
                Ret = PlatformFormatString(Buffer->brkp, BufferUnusedSize(Buffer), "\n");
            Buffer->brkp += Ret;
        }
        
        Texture->Filename.Clear();
    }
    
    file_internal void SerializeMaterial(mesh_converter *Converter, material_serial *Material)
    {
        jstring OutFile;
        jstring CmdReflFile;
        {
            jstring Interim = MATERIAL_OUTPUT_FOLDER + Material->Name;
            OutFile = Interim + ".mat";
            Interim.Clear();
            
            Interim = REFLECTION_OUTPUT_FOLDER + Material->Name;
            CmdReflFile = Interim + ".refl";
            
            Interim.Clear();
        }
        mprint("Material out file: %s\n", OutFile.GetCStr());
        
        FileBuffer Buffer;
        CreateFileBuffer(&Buffer, sizeof(material_serial));
        
        WriteToFileBuffer(&Buffer, "{Material}\n");
        WriteToFileBuffer(&Buffer, "ReflectionFile : str = \"%s\"\n", CmdReflFile.GetCStr());
        WriteToFileBuffer(&Buffer, "Name : str = \"%s\"\n", Material->Name.GetCStr());
        WriteToFileBuffer(&Buffer, "HasPBRMetallicRoughness : i32 = %d\n", Material->HasPBRMetallicRoughness);
        WriteToFileBuffer(&Buffer, "HasPBRSpecularGlossiness : i32 = %d\n", Material->HasPBRSpecularGlossiness);
        WriteToFileBuffer(&Buffer, "HasClearCoat : i32 = %d\n", Material->HasClearCoat);
        WriteToFileBuffer(&Buffer, "AlphaMode : i32 = %d\n", Material->AlphaMode);
        WriteToFileBuffer(&Buffer, "AlphaCutoff : r32 = %lf\n", Material->AlphaCutoff);
        WriteToFileBuffer(&Buffer, "DoubleSided : i32 = %d\n", Material->DoubleSided);
        WriteToFileBuffer(&Buffer, "Unlit : i32 = %d\n\n", Material->Unlit);
        
        WriteToFileBuffer(&Buffer, "{Metallic-Roughness}\n");
        if (Material->HasPBRMetallicRoughness)
        {
            WriteToFileBuffer(&Buffer, "BaseColorFactor : vec4 = [%lf,%lf,%lf,%lf]\n",
                              Material->BaseColorFactor.x,
                              Material->BaseColorFactor.y,
                              Material->BaseColorFactor.z,
                              Material->BaseColorFactor.w);
            WriteToFileBuffer(&Buffer, "MetallicFactor : r32 = %lf\n", Material->MetallicFactor);
            WriteToFileBuffer(&Buffer, "RoughnessFactor : r32 = %lf\n\n", Material->RoughnessFactor);
            
            SerializeTexture(&Buffer, &Material->BaseColorTexture, "Base Color Texture");
            SerializeTexture(&Buffer, &Material->MetallicRoughnessTexture, "Metallic Roughness Texture");
        }
        
        WriteToFileBuffer(&Buffer, "{Specular-Glossy}\n");
        if (Material->HasPBRSpecularGlossiness)
        {
            WriteToFileBuffer(&Buffer, "DiffuseFactor : vec4 = [%lf,%lf,%lf,%lf]\n",
                              Material->DiffuseFactor.x,
                              Material->DiffuseFactor.y,
                              Material->DiffuseFactor.z,
                              Material->DiffuseFactor.w);
            WriteToFileBuffer(&Buffer, "SpecularFactor : vec3 = [%lf,%lf,%lf]\n",
                              Material->DiffuseFactor.x,
                              Material->DiffuseFactor.y,
                              Material->DiffuseFactor.z);
            WriteToFileBuffer(&Buffer, "GlossinessFactor : r32 = %lf\n\n", Material->GlossinessFactor);
            
            SerializeTexture(&Buffer, &Material->DiffuseTexture, "Diffuse Texture");
            SerializeTexture(&Buffer, &Material->SpecularGlossinessTexture, "Specular Glossiness Texture");
        }
        
        WriteToFileBuffer(&Buffer, "{Clear Coat}\n");
        if (Material->HasClearCoat)
        {
            WriteToFileBuffer(&Buffer, "ClearCoatFactor : r32 = %lf\n", Material->ClearCoatFactor);
            WriteToFileBuffer(&Buffer, "ClearCoatRoughnessFactor : r32 = %lf\n\n", Material->ClearCoatRoughnessFactor);
            
            SerializeTexture(&Buffer, &Material->ClearCoatTexture, "Clear Coat Texture");
            SerializeTexture(&Buffer, &Material->ClearCoatRoughnessTexture, "Clear Coat Roughness Texture");
            SerializeTexture(&Buffer, &Material->ClearCoatNormalTexture, "Clear Coat Normal Texture");
        }
        
        SerializeTexture(&Buffer, &Material->NormalTexture, "Normal Texture");
        SerializeTexture(&Buffer, &Material->OcclusionTexture, "Occlusion Texture");
        SerializeTexture(&Buffer, &Material->EmissiveTexture, "Emissive Texture");
        
        WriteToFileBuffer(&Buffer, "{Shaders}\n");
        WriteToFileBuffer(&Buffer, "Vertex : str = \"data/shaders/shader.vert.spv\"\n");
        WriteToFileBuffer(&Buffer, "Fragment : str = \"data/shaders/shader.frag.spv\"\n");
        
        PlatformWriteBufferToFile(OutFile, Buffer.start, Buffer.brkp - Buffer.start);
        
        //~ Generate reflections files
        {
            jstring MatGenCmd;
            { // build the exe cmd
                jstring MatGenExe = pstring("mat_gen.exe ");
                jstring InterimA = MatGenExe + OutFile;
                jstring InterimB = InterimA + " ";
                jstring InterimC = InterimB + CmdReflFile;
                jstring InterimD = InterimC + " > ";
                jstring InterimE = InterimD + Material->Name;
                
                MatGenCmd = InterimE + "ReflLog.txt";
                
                MatGenExe.Clear();
                InterimA.Clear();
                InterimB.Clear();
                InterimC.Clear();
                InterimD.Clear();
                InterimE.Clear();
            }
            PlatformExecuteCommand(MatGenCmd);
            MatGenCmd.Clear();
        }
        
        // insert the mapping of the mat name to filename...
        Converter->MaterialFilenameMap.Insert(Material->Name, OutFile);
        
        CmdReflFile.Clear();
        //OutFile.Clear();
        Material->Name.Clear();
        DestroyFileBuffer(&Buffer);
    }
    
    file_internal void ConvertTexture(mesh_converter *Converter, cgltf_texture *CgTexture, texture_serial *Texture)
    {
        if (CgTexture->sampler) {
            Texture->MagFilter = CgTexture->sampler->mag_filter; // store?
            Texture->MinFilter = CgTexture->sampler->min_filter; // store?
            
            Texture->AddressModeU = CgTexture->sampler->wrap_s; // wrapS
            Texture->AddressModeV = CgTexture->sampler->wrap_t; // wrapT
            Texture->AddressModeW = CgTexture->sampler->wrap_s; // ??
        }
        
        // Load the CgTexture
        jstring TexturePath = InitJString(strlen(TEXTURE_OUTPUT_FOLDER) + strlen(CgTexture->image->uri));
        AddJString(TexturePath, TEXTURE_OUTPUT_FOLDER, CgTexture->image->uri);
        
        // TODO(Dustin): Copy the texture to the build/data/textures/ directory
        {
            jstring OrigPath = InitJString(((u32)strlen(CgTexture->image->uri)) + Converter->Directory.len);
            AddJString(OrigPath, Converter->Directory, CgTexture->image->uri);
            
            PlatformCopyFileIfChanged(TexturePath.GetCStr(), OrigPath.GetCStr());
            
            OrigPath.Clear();
        }
        
        // TODO(Dustin): Assgin the texture path to be the new directory
        Texture->Filename = TexturePath; // use the old path...for now
    }
    
    file_internal void ConvertMaterials(mesh_converter *Converter)
    {
        u32 MaterialCount = Converter->Data->materials_count;
        
        // materials are inserted into the map during serialization - see "SerializeMaterial" function.
        Converter->MaterialFilenameMap = HashTable<jstring, jstring>(MaterialCount);
        
        for (u32 MaterialIdx = 0; MaterialIdx < MaterialCount; ++MaterialIdx)
        {
            cgltf_material CgMaterial = Converter->Data->materials[MaterialIdx];
            material_serial Material = {};
            
            if (CgMaterial.name) {
                Material.Name = InitJString(CgMaterial.name);
                mprint("Material %s.\n", Material.Name.GetCStr());
            }
            else
            {
                mprinte("Material does not have a name!\n");
                continue;
            }
            
            Material.HasPBRMetallicRoughness  = CgMaterial.has_pbr_metallic_roughness;
            Material.HasPBRSpecularGlossiness = CgMaterial.has_pbr_specular_glossiness;
            Material.HasClearCoat             = CgMaterial.has_clearcoat;
            
            // NOTE(Dustin): This is a very silly if-statement, but I want to be able to detect
            // if there will ever be a combination of these settings
            if ((Material.HasPBRMetallicRoughness  && Material.HasPBRSpecularGlossiness) ||
                (Material.HasPBRMetallicRoughness  && Material.HasClearCoat) ||
                (Material.HasPBRSpecularGlossiness && Material.HasClearCoat) ||
		(Material.HasPBRMetallicRoughness  && Material.HasPBRSpecularGlossiness && Material.HasClearCoat))
            {
                printf("%s Material has a combination of material types!\n", Material.Name.GetCStr());
            }
            
            if (Material.HasPBRMetallicRoughness) {
                cgltf_pbr_metallic_roughness cmr = CgMaterial.pbr_metallic_roughness;
                
                cgltf_texture_view bctv = cmr.base_color_texture;
                cgltf_texture_view mrtv = cmr.metallic_roughness_texture;
                
                if (bctv.texture)
                {
                    cgltf_texture *bct = bctv.texture;
                    ConvertTexture(Converter, bct, &Material.BaseColorTexture);
                }
                
                if (mrtv.texture)
                {
                    cgltf_texture *mrt = mrtv.texture;
                    ConvertTexture(Converter, mrt, &Material.MetallicRoughnessTexture);
                }
                
                Material.BaseColorFactor = MakeVec4(cmr.base_color_factor); // Will this always be a vec4?
                Material.MetallicFactor  = cmr.metallic_factor;
                Material.RoughnessFactor = cmr.roughness_factor;
            }
            else if (Material.HasPBRSpecularGlossiness)
            {
                cgltf_pbr_specular_glossiness csg = CgMaterial.pbr_specular_glossiness;
                
                cgltf_texture_view dv = csg.diffuse_texture;
                cgltf_texture_view sgtv = csg.specular_glossiness_texture;
                
                if (dv.texture)
                {
                    cgltf_texture *dt = dv.texture;
                    ConvertTexture(Converter, dt, &Material.DiffuseTexture);
                }
                
                if (sgtv.texture)
                {
                    cgltf_texture *sgt = sgtv.texture;
                    ConvertTexture(Converter, sgt, &Material.SpecularGlossinessTexture);
                }
                
                Material.DiffuseFactor    = MakeVec4(csg.diffuse_factor);
                Material.SpecularFactor   = MakeVec3(csg.specular_factor);
                Material.GlossinessFactor = csg.glossiness_factor;;
            }
            else if (Material.HasClearCoat)
            {
                cgltf_clearcoat cc = CgMaterial.clearcoat;
                
                cgltf_texture_view cctv = cc.clearcoat_texture;
                cgltf_texture_view ccrtv = cc.clearcoat_roughness_texture;
                cgltf_texture_view ccntv = cc.clearcoat_normal_texture;
                
                if (cctv.texture)
                {
                    cgltf_texture *cct = cctv.texture;
                    ConvertTexture(Converter, cct, &Material.ClearCoatTexture);
                }
                
                if (ccrtv.texture)
                {
                    cgltf_texture *ccrt = ccrtv.texture;
                    ConvertTexture(Converter, ccrt, &Material.ClearCoatRoughnessTexture);
                }
                
                if (ccntv.texture)
                {
                    cgltf_texture *ccnt = ccrtv.texture;
                    ConvertTexture(Converter, ccnt, &Material.ClearCoatNormalTexture);
                }
                
                Material.ClearCoatFactor = cc.clearcoat_factor;
                Material.ClearCoatRoughnessFactor = cc.clearcoat_roughness_factor;
            }
            
            { // Normal texture
                cgltf_texture_view ntv = CgMaterial.normal_texture;
                cgltf_texture *nt = ntv.texture;
                
                if (nt)
                    ConvertTexture(Converter, nt, &Material.NormalTexture);
            }
            
            { // Occlusion Texture
                cgltf_texture_view ntv = CgMaterial.occlusion_texture;
                cgltf_texture *nt = ntv.texture;
                
                if (nt)
                    ConvertTexture(Converter, nt, &Material.OcclusionTexture);
            }
            
            { // Emissive Texture
                cgltf_texture_view ntv = CgMaterial.emissive_texture;
                cgltf_texture *nt = ntv.texture;
                
                if (nt)
                    ConvertTexture(Converter, nt, &Material.EmissiveTexture);
            }
            
            SerializeMaterial(Converter, &Material);
            
            Material.Name.Clear();
        }
    }
    
    void ConvertGlTF(jstring Filename)
    {
        mesh_converter Converter = {};
        
        const char *cfile = Filename.GetCStr();
        const char* ptr = strrchr(cfile, '/');
        if (ptr == nullptr) {
            ptr = strrchr(cfile, '\\');
            
            if (ptr == nullptr) {
                mprinte("Could not find directory!\n");
                return;
            }
        }
        
        ptr++;
        
        size_t len = ptr - cfile;
        char *dirname = (char*)talloc(len);
        strncpy(dirname, cfile, len);
        
        Converter.Directory = InitJString(dirname, (u32)len);
        
        const char* Extension = strrchr(cfile, '.');
        if (ptr == nullptr) {
            mprinte("Could not find the extensions!\n");
            return;
        }
        
        Extension++;
        
        jstring CorrectExtension = InitJString("gltf");
        
        if (!(CorrectExtension == Extension))
        {
            mprinte("File extension for a model is not a glTF file!\n");
            return;
        }
        
        const char *ModelName = Filename.GetCStr() + len;
        u32 FilenameLen =  Extension - ModelName - 1;
        jstring ModelFilename = InitJString(ModelName, FilenameLen);;
        
        jstring OutputModelFilename;
        jstring OutputBinaryFilename;
        {
            jstring TempModelPath = MODEL_OUTPUT_FOLDER + ModelFilename;
            jstring TempBinPath   = BINARY_OUTPUT_FOLDER + ModelFilename;
            
            OutputModelFilename  = TempModelPath + MODEL_EXTENSION;
            OutputBinaryFilename = TempBinPath   + BINARY_EXTENSION;
            
            TempModelPath.Clear();
            TempBinPath.Clear();
        }
        
        CorrectExtension.Clear();
        ModelFilename.Clear();
        
        cgltf_options options = {};
        Converter.Data = nullptr;
        cgltf_result result = cgltf_parse_file(&options, Filename.GetCStr(), &Converter.Data);
        if (result != cgltf_result_success)
        {
            mprinte("Failed to load mesh %s!\n", Filename.GetCStr());
            return;
        }
        
        // load the buffers
        // NOTE(Dustin): Maybe in the future, stream the data directly to the gpu?
        result = cgltf_load_buffers(&options, Converter.Data, Filename.GetCStr());
        if (result != cgltf_result_success)
        {
            mprinte("Failed to load mesh buffers %s!\n", Filename.GetCStr());
            return;
        }
        
        //~ Parse and Serialize Materials
        ConvertMaterials(&Converter);
        
        //~ Convert the Scene
        
        Converter.NodeIdx         = 0;
        Converter.MeshIdx         = 0;
        Converter.PrimitiveIdx    = 0;
        Converter.SerialNodeList  = talloc<model_node_serial>(Converter.Data->nodes_count);
        Converter.SerialSceneList = talloc<scene_serial>(Converter.Data->scenes_count);
        Converter.SerialMeshList  = talloc<mesh_serial>(Converter.Data->meshes_count);
        CreateFileBuffer(&Converter.PrimitiveDataBlock);
        
        // Determine the amount of primitives in the file
        u32 PrimitivesCount = 0;
        for (u32 Mesh = 0; Mesh < Converter.Data->meshes_count; ++Mesh)
        {
            PrimitivesCount += Converter.Data->meshes[Mesh].primitives_count;
        }
        
        Converter.SerialPrimitiveList = talloc<primitive_serial>(PrimitivesCount);
        
        for (u32 Scene = 0; Scene < Converter.Data->scenes_count; ++Scene)
            ConvertScene(&Converter, &Converter.Data->scenes[Scene]);
        
        //~ Serialize the mesh
        
        u32 ExpectedFileSize = sizeof(model_node_serial) * Converter.NodeIdx + sizeof(u32);
        ExpectedFileSize    += sizeof(primitive_serial) * Converter.PrimitiveIdx + sizeof(u32);
        ExpectedFileSize    += sizeof(mesh_serial) * Converter.MeshIdx + sizeof(u32);
        
        FileBuffer Buffer;
        CreateFileBuffer(&Buffer, ExpectedFileSize);
        
        JStringToBinaryBuffer(&Buffer, OutputBinaryFilename);
        
        // Serialize primitives
        Int32ToBinaryBuffer(&Buffer, &Converter.PrimitiveIdx, 1);
        
        for (i32 Primitive = 0; Primitive < Converter.PrimitiveIdx; ++Primitive)
        {
            // Data info
            UInt64ToBinaryBuffer(&Buffer, &Converter.SerialPrimitiveList[Primitive].Offset, 1);
            UInt32ToBinaryBuffer(&Buffer, &Converter.SerialPrimitiveList[Primitive].IndexCount, 1);
            UInt32ToBinaryBuffer(&Buffer, &Converter.SerialPrimitiveList[Primitive].IndexStride, 1);
            UInt32ToBinaryBuffer(&Buffer, &Converter.SerialPrimitiveList[Primitive].VertexCount, 1);
            UInt32ToBinaryBuffer(&Buffer, &Converter.SerialPrimitiveList[Primitive].VertexStride, 1);
            BoolToBinaryBuffer(&Buffer, &Converter.SerialPrimitiveList[Primitive].IsSkinned, 1);
            
            // Min/Max data for the primitve
            FloatToBinaryBuffer(&Buffer, Converter.SerialPrimitiveList[Primitive].Min.data, 3);
            FloatToBinaryBuffer(&Buffer, Converter.SerialPrimitiveList[Primitive].Max.data, 3);
            
            // Material Info
            JStringToBinaryBuffer(&Buffer, Converter.SerialPrimitiveList[Primitive].MaterialFile);
            JStringToBinaryBuffer(&Buffer, Converter.SerialPrimitiveList[Primitive].MaterialName);
            
            // The MaterialMapping Table owns the string memory, so don't have to clear it here...
            //Converter.SerialPrimitiveList[Primitive].MaterialFile.Clear();
            Converter.SerialPrimitiveList[Primitive].MaterialName.Clear();
        }
        
        // Serialize the meshes
        Int32ToBinaryBuffer(&Buffer, &Converter.MeshIdx, 1);
        
        for (i32 Mesh = 0; Mesh < Converter.MeshIdx; ++Mesh)
        {
            JStringToBinaryBuffer(&Buffer, Converter.SerialMeshList[Mesh].MeshName);
            
            UInt64ToBinaryBuffer(&Buffer, &Converter.SerialMeshList[Mesh].PrimitivesCount, 1);
            Int32ToBinaryBuffer(&Buffer, Converter.SerialMeshList[Mesh].PrimitivesIdx,
                                Converter.SerialMeshList[Mesh].PrimitivesCount);
            
            Converter.SerialMeshList[Mesh].MeshName.Clear();
        }
        
        // Serialize the node heirarchy
        Int32ToBinaryBuffer(&Buffer, &Converter.NodeIdx, 1);
        
        for (i32 Node = 0; Node < Converter.NodeIdx; ++Node)
        {
            JStringToBinaryBuffer(&Buffer, Converter.SerialNodeList[Node].Name);
            
            Int32ToBinaryBuffer(&Buffer, &Converter.SerialNodeList[Node].ParentIdx, 1);
            
            UInt64ToBinaryBuffer(&Buffer, &Converter.SerialNodeList[Node].ChildrenCount, 1);
            Int32ToBinaryBuffer(&Buffer, Converter.SerialNodeList[Node].ChildrenIndices,
                                Converter.SerialNodeList[Node].ChildrenCount);
            
            FloatToBinaryBuffer(&Buffer, Converter.SerialNodeList[Node].Translation.data, 3);
            FloatToBinaryBuffer(&Buffer, Converter.SerialNodeList[Node].Scale.data, 3);
            FloatToBinaryBuffer(&Buffer, Converter.SerialNodeList[Node].Rotation.data, 4);
            
            Int32ToBinaryBuffer(&Buffer, &Converter.SerialNodeList[Node].MeshIdx, 1);
            //JStringToBinaryBuffer(&Buffer, Converter.SerialNodeList[Node].MeshName);
            
            Converter.SerialNodeList[Node].Name.Clear();
            Converter.SerialNodeList[Node].MeshName.Clear();
        }
        
        // serialize the disjoint set
        Int32ToBinaryBuffer(&Buffer, &Converter.SceneIdx, 1);
        for (i32 Scene = 0; Scene < Converter.SceneIdx; ++Scene)
        {
            UInt32ToBinaryBuffer(&Buffer, &Converter.SerialSceneList[Scene].NodesCount, 1);
            Int32ToBinaryBuffer(&Buffer, Converter.SerialSceneList[Scene].NodesIdx,
                                Converter.SerialSceneList[Scene].NodesCount);
        }
        
        // Finally...write to the file
        PlatformWriteBufferToFile(OutputModelFilename, Buffer.start, Buffer.brkp - Buffer.start);
        
        // Serialize the binary buffer
        // NOTE(Dustin): Go back and check this actually works :)
        PlatformWriteBufferToFile(OutputBinaryFilename,
                                  Converter.PrimitiveDataBlock.start,
                                  Converter.PrimitiveDataBlock.brkp - Converter.PrimitiveDataBlock.start);
        
        //~ Clean Resources
        // ok, this is dumb...
        HashTable<jstring, jstring>::Entry *MatEntries = Converter.MaterialFilenameMap.GetEntries();
        for (u32 Idx = 0; Idx < Converter.MaterialFilenameMap.Capacity; ++Idx)
        {
            if (!MatEntries[Idx].IsEmpty) MatEntries[Idx].Value.Clear();
        }
        Converter.MaterialFilenameMap.Reset();
        
        DestroyFileBuffer(&Converter.PrimitiveDataBlock);
        DestroyFileBuffer(&Buffer);
        Converter.Directory.Clear();
        OutputModelFilename.Clear();
        OutputBinaryFilename.Clear();
    }
} // masset
