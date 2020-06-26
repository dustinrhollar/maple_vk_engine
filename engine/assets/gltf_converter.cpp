
file_global const char *MODEL_OUTPUT_FOLDER      = "data/models/models/";
file_global const char *BINARY_OUTPUT_FOLDER     = "data/models/binaries/";
file_global const char *MATERIAL_OUTPUT_FOLDER   = "data/materials/";
file_global const char *TEXTURE_OUTPUT_FOLDER    = "data/textures/";
file_global const char *REFLECTION_OUTPUT_FOLDER = "data/mat_refl/";
file_global const char *BINARY_EXTENSION         = ".bin";
file_global const char *MODEL_EXTENSION          = ".model";

file_internal void SerializeTexture(file_t File, texture_serial *Texture, const char *TextureHeader);
file_internal void SerializeMaterial(mesh_converter *Converter, material_serial *Material);
file_internal i32 ConvertPrimitive(mesh_converter *Converter, cgltf_primitive *CgPrimitive);
file_internal i32 ConvertMesh(mesh_converter *Converter, cgltf_mesh *CgMesh);
file_internal i32 ConvertNode(mesh_converter *Converter, cgltf_node *CgNode);
file_internal i32 ConvertScene(mesh_converter *Converter, cgltf_scene *CgScene);
file_internal void ConvertTexture(mesh_converter *Converter, cgltf_texture *CgTexture, texture_serial *Texture);
file_internal void ConvertMaterials(mesh_converter *Converter);


file_internal void SerializeTexture(file_t File, texture_serial *Texture, const char *TextureHeader)
{
    PlatformWriteToFile(File, "{%s}\n", TextureHeader);
    PlatformWriteToFile(File, "Filename : str = \"%s\"\n", GetStr(&Texture->Filename));
    PlatformWriteToFile(File, "MinFilter : i32 = %d\n", Texture->MinFilter);
    PlatformWriteToFile(File, "MagFilter : i32 = %d\n", Texture->MagFilter);
    PlatformWriteToFile(File, "AddressModeU : i32 = %d\n", Texture->AddressModeU);
    PlatformWriteToFile(File, "AddressModeV : i32 = %d\n", Texture->AddressModeV);
    PlatformWriteToFile(File, "AddressModeW : i32 = %d\n\n", Texture->AddressModeW);
    
    MstringFree(&Texture->Filename);
}

file_internal void SerializeMaterial(mesh_converter *Converter, material_serial *Material)
{
    mstring OutFile     = {};
    mstring CmdReflFile = {};
    {
        MstringAdd(&OutFile, &OutFile, MATERIAL_OUTPUT_FOLDER, strlen(MATERIAL_OUTPUT_FOLDER));
        MstringAdd(&OutFile, &OutFile, &Material->Name);
        MstringAdd(&OutFile, &OutFile, ".mat", 4);
        
        MstringAdd(&CmdReflFile, &CmdReflFile, MATERIAL_OUTPUT_FOLDER, strlen(MATERIAL_OUTPUT_FOLDER));
        MstringAdd(&CmdReflFile, &CmdReflFile, &Material->Name);
        MstringAdd(&CmdReflFile, &CmdReflFile, ".refl", 5);
    }
    mprint("Material out file: %s\n", GetStr(&OutFile));
    
    file_t MatFile = PlatformOpenFile(GetStr(&OutFile));
    
    PlatformWriteToFile(MatFile, "{Material}\n");
    //PlatformWriteToFile(MatFile, "ReflectionFile : str = \"%s\"\n", GetStr(&CmdReflFile));
    PlatformWriteToFile(MatFile, "Name : str = \"%s\"\n", GetStr(&Material->Name));
    PlatformWriteToFile(MatFile, "HasPBRMetallicRoughness : i32 = %d\n", Material->HasPBRMetallicRoughness);
    PlatformWriteToFile(MatFile, "HasPBRSpecularGlossiness : i32 = %d\n", Material->HasPBRSpecularGlossiness);
    PlatformWriteToFile(MatFile, "HasClearCoat : i32 = %d\n", Material->HasClearCoat);
    PlatformWriteToFile(MatFile, "AlphaMode : i32 = %d\n", Material->AlphaMode);
    PlatformWriteToFile(MatFile, "AlphaCutoff : r32 = %lf\n", Material->AlphaCutoff);
    PlatformWriteToFile(MatFile, "DoubleSided : i32 = %d\n", Material->DoubleSided);
    PlatformWriteToFile(MatFile, "Unlit : i32 = %d\n\n", Material->Unlit);
    
    PlatformWriteToFile(MatFile, "{Metallic-Roughness}\n");
    if (Material->HasPBRMetallicRoughness)
    {
        PlatformWriteToFile(MatFile, "BaseColorFactor : vec4 = [%lf,%lf,%lf,%lf]\n",
                            Material->BaseColorFactor.x,
                            Material->BaseColorFactor.y,
                            Material->BaseColorFactor.z,
                            Material->BaseColorFactor.w);
        PlatformWriteToFile(MatFile, "MetallicFactor : r32 = %lf\n", Material->MetallicFactor);
        PlatformWriteToFile(MatFile, "RoughnessFactor : r32 = %lf\n\n", Material->RoughnessFactor);
        
        SerializeTexture(MatFile, &Material->BaseColorTexture, "Base Color Texture");
        SerializeTexture(MatFile, &Material->MetallicRoughnessTexture, "Metallic Roughness Texture");
    }
    
    PlatformWriteToFile(MatFile, "{Specular-Glossy}\n");
    if (Material->HasPBRSpecularGlossiness)
    {
        PlatformWriteToFile(MatFile, "DiffuseFactor : vec4 = [%lf,%lf,%lf,%lf]\n",
                            Material->DiffuseFactor.x,
                            Material->DiffuseFactor.y,
                            Material->DiffuseFactor.z,
                            Material->DiffuseFactor.w);
        PlatformWriteToFile(MatFile, "SpecularFactor : vec3 = [%lf,%lf,%lf]\n",
                            Material->DiffuseFactor.x,
                            Material->DiffuseFactor.y,
                            Material->DiffuseFactor.z);
        PlatformWriteToFile(MatFile, "GlossinessFactor : r32 = %lf\n\n", Material->GlossinessFactor);
        
        SerializeTexture(MatFile, &Material->DiffuseTexture, "Diffuse Texture");
        SerializeTexture(MatFile, &Material->SpecularGlossinessTexture, "Specular Glossiness Texture");
    }
    
    PlatformWriteToFile(MatFile, "{Clear Coat}\n");
    if (Material->HasClearCoat)
    {
        PlatformWriteToFile(MatFile, "ClearCoatFactor : r32 = %lf\n", Material->ClearCoatFactor);
        PlatformWriteToFile(MatFile, "ClearCoatRoughnessFactor : r32 = %lf\n\n", Material->ClearCoatRoughnessFactor);
        
        SerializeTexture(MatFile, &Material->ClearCoatTexture, "Clear Coat Texture");
        SerializeTexture(MatFile, &Material->ClearCoatRoughnessTexture, "Clear Coat Roughness Texture");
        SerializeTexture(MatFile, &Material->ClearCoatNormalTexture, "Clear Coat Normal Texture");
    }
    
    SerializeTexture(MatFile, &Material->NormalTexture, "Normal Texture");
    SerializeTexture(MatFile, &Material->OcclusionTexture, "Occlusion Texture");
    SerializeTexture(MatFile, &Material->EmissiveTexture, "Emissive Texture");
    
    PlatformWriteToFile(MatFile, "{Shaders}\n");
    PlatformWriteToFile(MatFile, "Vertex : str = \"\"\n");
    PlatformWriteToFile(MatFile, "Fragment : str = \"\"\n");
    
    PlatformFlushFile(MatFile);
    PlatformCloseFile(MatFile);
    
    MstringFree(&OutFile);
    MstringFree(&CmdReflFile);
}

file_internal i32 ConvertPrimitive(mesh_converter *Converter, cgltf_primitive *CgPrimitive)
{
    primitive_serial Primitive = {};
    Primitive.PrimitiveIdx = Converter->PrimitiveIdx++;
    Primitive.VertexStride = sizeof(vertex);
    Primitive.IndexStride  = sizeof(u32);
    Primitive.MaterialFile = {};
    
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
        Mstring(&Primitive.MaterialName, CgPrimitive->material->name, strlen(CgPrimitive->material->name));
        
        // TODO(Dustin): Check the material list to make sure it was loaded...
        mstring Filename = {};
        
        if (Filename.Len > 0)
        {
            Primitive.MaterialFile = Filename;
        }
        else
            mprinte("Could not find a material with the name \"%s\"!\n", GetStr(&Primitive.MaterialName));
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
    
    // NOTE(Dustin): Hardcoded vertex type - does not check if skinned
    u64 DataSize = sizeof(u32) * Primitive.IndexCount + sizeof(vertex) * Primitive.VertexCount;
    
    // TODO(Dustin): Temporary Checks for file size
#if 1
    if (sizeof(u32) * Primitive.IndexCount > _2MB)
    {
        mprinte("Size of indices is over 2MB!\n");
    }
    
    if (sizeof(vertex) * Primitive.VertexCount > _2MB)
    {
        mprinte("Size of vertices is over 2MB!\n");
    }
    
    if (DataSize > _2MB)
    {
        mprinte("total data sizeis over 2MB!\n");
    }
#endif
    
    Primitive.Offset = Converter->CurrentBinaryFileOffset;
    
    u32 IndicesOffset  = Primitive.Offset;
    u32 VerticesOffset = IndicesOffset + sizeof(u32) * Primitive.IndexCount;
    
    //Converter->PrimitiveDataBlock.brkp += VerticesOffset + sizeof(Vertex) * Primitive.VertexCount;
    
    bool HasPosition = false;
    bool HasNormals  = false;
    bool HasColor    = false;
    bool HasUVs      = false;
    bool HasWeights  = false;
    bool HasJoints   = false;
    
    float *position_buffer = nullptr;
    float *normal_buffer   = nullptr;
    float *color_buffer    = nullptr;
    float *uv0_buffer      = nullptr;
    float *weights_buffer  = nullptr;
    float *joints_buffer   = nullptr;
    
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
                HasColor = true;
                color_buffer = (float*)(buffer + accessor->offset + buffer_view->offset);
            } break;
            
            case cgltf_attribute_type_texcoord:
            {
                HasUVs = true;
                uv0_buffer = (float*)(buffer + accessor->offset + buffer_view->offset);
            } break;
            
            case cgltf_attribute_type_weights:
            {
                HasUVs = true;
                weights_buffer = (float*)(buffer + accessor->offset + buffer_view->offset);
            } break;
            
            case cgltf_attribute_type_joints:
            {
                HasUVs = true;
                joints_buffer = (float*)(buffer + accessor->offset + buffer_view->offset);
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
                for (int i = 0; i < Primitive.IndexCount; ++i)
                {
                    u32 Value = indices_buffer[i];
                    PlatformWriteBinaryToFile(Converter->BinaryDataFile, "u32",
                                              &Value, 1);
                }
            } break;
            
            case cgltf_component_type_r_8u:
            {
                u8 *indices_buffer = (u8*)unknown_indices_buffer;
                
                for (int i = 0; i < Primitive.IndexCount; ++i)
                {
                    u32 Value = indices_buffer[i];
                    PlatformWriteBinaryToFile(Converter->BinaryDataFile, "u32",
                                              &Value, 1);
                }
            } break;
            
            case cgltf_component_type_r_32u:
            {
                u32 *indices_buffer = (u32*)unknown_indices_buffer;
                PlatformWriteBinaryToFile(Converter->BinaryDataFile, "u32",
                                          indices_buffer, Primitive.IndexCount);
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
        //vertex *Vertices = (vertex*)((char*)Converter->PrimitiveDataBlock.start + VerticesOffset);
        
        vec2 dummy_vec2 = { 0.0f, 0.0f };
        vec3 dummy_vec3 = { 0.f, 0.0f, 0.0f };
        vec4 dummy_vec4 = { 0.5f, 0.5f, 0.5f, 1.0f };
        
        for (size_t idx = 0; idx < Primitive.VertexCount; ++idx)
        {
            vertex Vertex = {};
            Vertex.Position = MakeVec3(position_buffer + idx * 3);
            Vertex.Normals  = (normal_buffer) ? MakeVec3(normal_buffer + idx * 3) : dummy_vec3;
            Vertex.Color    = (color_buffer)  ? MakeVec4(color_buffer + idx * 4)  : dummy_vec4;
            Vertex.Tex0     = (uv0_buffer)    ? MakeVec2(uv0_buffer + idx * 2)    : dummy_vec2;
            
            PlatformWriteBinaryStreamToFile(Converter->BinaryDataFile, &Vertex, sizeof(vertex));
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
    
    return Primitive.PrimitiveIdx;}

file_internal i32 ConvertMesh(mesh_converter *Converter, cgltf_mesh *CgMesh)
{
    mesh_serial Mesh     = {};
    Mesh.MeshIdx         = Converter->MeshIdx++;
    Mesh.PrimitivesCount = CgMesh->primitives_count;
    Mesh.PrimitivesIdx   = halloc<i32>(Converter->Heap, Mesh.PrimitivesCount);
    
    if (CgMesh->name)
        Mstring(&Mesh.MeshName, CgMesh->name, strlen(CgMesh->name));
    
    for (int i = 0; i < CgMesh->primitives_count; ++i)
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
    Node.Name      = {};
    Node.MeshName  = {};
    Node.MeshIdx   = -1;
    Node.ParentIdx = -1;
    
    if (CgNode->name)
    {
        Mstring(&Node.Name, CgNode->name, strlen(CgNode->name));
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
    Node.ChildrenIndices = halloc<i32>(Converter->Heap, Node.ChildrenCount);
    
    for (int i = 0; i < Node.ChildrenCount; ++i)
    {
        i32 ChildIdx = ConvertNode(Converter, CgNode->children[i]);
        Node.ChildrenIndices[i] = ChildIdx;
    }
    
    Converter->SerialNodeList[Node.NodeIdx] = Node;
    for (int i = 0; i < Node.ChildrenCount; ++i)
    {
        i32 ChildIdx = Node.ChildrenIndices[i];
        Converter->SerialNodeList[ChildIdx].ParentIdx = Node.NodeIdx;
    }
    
    return Node.NodeIdx;}

file_internal i32 ConvertScene(mesh_converter *Converter, cgltf_scene *CgScene)
{
    scene_serial Scene = {};
    Scene.SceneIdx     = Converter->SceneIdx++;
    Scene.NodesCount   = (u32)CgScene->nodes_count;
    Scene.NodesIdx     = halloc<i32>(Converter->Heap, Scene.NodesCount);
    
    for (int i = 0; i < Scene.NodesCount; ++i)
    {
        i32 Idx = ConvertNode(Converter, CgScene->nodes[i]);
        Scene.NodesIdx[i] = Idx;
    }
    
    Converter->SerialSceneList[Scene.SceneIdx] = Scene;
    
    return Scene.SceneIdx;
}

file_internal void ConvertTexture(mesh_converter *Converter, cgltf_texture *CgTexture, texture_serial *Texture)
{
    if (CgTexture->sampler)
    {
        Texture->MagFilter = CgTexture->sampler->mag_filter; // store?
        Texture->MinFilter = CgTexture->sampler->min_filter; // store?
        
        Texture->AddressModeU = CgTexture->sampler->wrap_s; // wrapS
        Texture->AddressModeV = CgTexture->sampler->wrap_t; // wrapT
        Texture->AddressModeW = CgTexture->sampler->wrap_s; // ??
    }
    
    mstring TexturePath;
    Mstring(&TexturePath, NULL, strlen(TEXTURE_OUTPUT_FOLDER) + strlen(CgTexture->image->uri));
    StringAdd(&TexturePath, TEXTURE_OUTPUT_FOLDER, strlen(TEXTURE_OUTPUT_FOLDER),
              CgTexture->image->uri, strlen(CgTexture->image->uri));
    
    {
        mstring OrigPath;
        Mstring(&OrigPath, NULL, strlen(CgTexture->image->uri) + Converter->Directory.Len);
        MstringAdd(&OrigPath, &Converter->Directory, CgTexture->image->uri, (u32)strlen(CgTexture->image->uri));
        
        PlatformCopyFileIfChanged(GetStr(&TexturePath), GetStr(&OrigPath));
        
        MstringFree(&OrigPath);
    }
    
    Texture->Filename = TexturePath;
}

file_internal void ConvertMaterials(mesh_converter *Converter)
{
    u32 MaterialCount = Converter->Data->materials_count;
    
    Converter->MaterialNameList = halloc<mstring>(Converter->Heap, MaterialCount);
    Converter->MaterialNameListIdx = 0;
    
    for (u32 MaterialIdx = 0; MaterialIdx < MaterialCount; ++MaterialIdx)
    {
        cgltf_material CgMaterial = Converter->Data->materials[MaterialIdx];
        material_serial Material = {};
        
        if (CgMaterial.name) {
            Mstring(&Material.Name, CgMaterial.name, strlen(CgMaterial.name));
            Mstring(&Converter->MaterialNameList[Converter->MaterialNameListIdx++],
                    CgMaterial.name, strlen(CgMaterial.name));
            
            mprint("Material %s is being converted.\n", GetStr(&Material.Name));
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
        if (Material.HasPBRMetallicRoughness  && Material.HasPBRSpecularGlossiness ||
            Material.HasPBRMetallicRoughness  && Material.HasClearCoat ||
            Material.HasPBRSpecularGlossiness && Material.HasClearCoat ||
            Material.HasPBRMetallicRoughness  && Material.HasPBRSpecularGlossiness && Material.HasClearCoat)
        {
            mprinte("%s Material has a combination of material types!\n", GetStr(&Material.Name));
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
        
        SerializeMaterial(Converter, &Material);
        
        MstringFree(&Material.Name);
    }
}

void ConvertGltfMesh(tag_block_t Heap, const char *Filename)
{
    mesh_converter Converter = {};
    Converter.Heap = Heap;
    
    mstring OutputModelFilename  = {};
    mstring OutputBinaryFilename = {};
    
    const char *cfile = Filename;
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
    
    Converter.Directory = {};
    Mstring(&Converter.Directory, cfile, len);
    
    const char* Extension = strrchr(cfile, '.');
    Extension++;
    
    const char *ModelName = Filename + len;
    u32 FilenameLen =  Extension - ModelName - 1;
    
    mstring ModelFilename = {};
    Mstring(&ModelFilename, ModelName, FilenameLen);
    {
        MstringAdd(&OutputModelFilename, &OutputModelFilename, MODEL_OUTPUT_FOLDER, strlen(MODEL_OUTPUT_FOLDER));
        MstringAdd(&OutputModelFilename, &OutputModelFilename, &ModelFilename);
        MstringAdd(&OutputModelFilename, &OutputModelFilename, MODEL_EXTENSION, strlen(MODEL_EXTENSION));
        
        MstringAdd(&OutputBinaryFilename, &OutputBinaryFilename, BINARY_OUTPUT_FOLDER, strlen(BINARY_OUTPUT_FOLDER));
        MstringAdd(&OutputBinaryFilename, &OutputBinaryFilename, &ModelFilename);
        MstringAdd(&OutputBinaryFilename, &OutputBinaryFilename, BINARY_EXTENSION, strlen(BINARY_EXTENSION));
    }
    
    MstringFree(&ModelFilename);
    
    Converter.BinaryDataFile = PlatformOpenFile(GetStr(&OutputBinaryFilename));
    if (!Converter.BinaryDataFile) mprinte("Unable to create the output binary file!\n");
    Converter.CurrentBinaryFileOffset = 0;
    
    cgltf_options options = {0};
    Converter.Data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, Filename, &Converter.Data);
    if (result != cgltf_result_success)
    {
        mprinte("Failed to load mesh %s!\n", Filename);
        return;
    }
    
    // load the buffers
    // NOTE(Dustin): Maybe in the future, stream the data directly to the gpu?
    result = cgltf_load_buffers(&options, Converter.Data, Filename);
    if (result != cgltf_result_success)
    {
        mprinte("Failed to load mesh buffers %s!\n", Filename);
        return;
    }
    
    //~ Parse and Serialize the materials
    ConvertMaterials(&Converter);
    
    //~ Convert the Scene
    
    Converter.NodeIdx         = 0;
    Converter.MeshIdx         = 0;
    Converter.PrimitiveIdx    = 0;
    Converter.SerialNodeList  = halloc<model_node_serial>(Heap, Converter.Data->nodes_count);
    Converter.SerialSceneList = halloc<scene_serial>(Heap, Converter.Data->scenes_count);
    Converter.SerialMeshList  = halloc<mesh_serial>(Heap, Converter.Data->meshes_count);
    
    // Determine the amount of primitives in the file
    u32 PrimitivesCount = 0;
    for (u32 Mesh = 0; Mesh < Converter.Data->meshes_count; ++Mesh)
    {
        PrimitivesCount += Converter.Data->meshes[Mesh].primitives_count;
    }
    
    Converter.SerialPrimitiveList = halloc<primitive_serial>(Heap, PrimitivesCount);
    
    for (u32 Scene = 0; Scene < Converter.Data->scenes_count; ++Scene)
        ConvertScene(&Converter, &Converter.Data->scenes[Scene]);
    
    //~ Serialize the model
    file_t ModelFile = PlatformOpenFile(GetStr(&OutputModelFilename));
    if (!ModelFile) mprinte("Unable to create the output binary file!\n");
    
    PlatformWriteBinaryToFile(ModelFile, "m", &OutputBinaryFilename, 1);
    PlatformWriteBinaryToFile(ModelFile, "i32", &Converter.PrimitiveIdx, 1);
    
    for (u32 Primitive = 0; Primitive < Converter.PrimitiveIdx; ++Primitive)
    {
        PlatformWriteBinaryToFile(ModelFile, "u64", &Converter.SerialPrimitiveList[Primitive].Offset, 1);
        PlatformWriteBinaryToFile(ModelFile, "u32", &Converter.SerialPrimitiveList[Primitive].IndexCount, 1);
        PlatformWriteBinaryToFile(ModelFile, "u32", &Converter.SerialPrimitiveList[Primitive].IndexStride, 1);
        PlatformWriteBinaryToFile(ModelFile, "u32", &Converter.SerialPrimitiveList[Primitive].VertexCount, 1);
        PlatformWriteBinaryToFile(ModelFile, "u32", &Converter.SerialPrimitiveList[Primitive].VertexStride, 1);
        PlatformWriteBinaryToFile(ModelFile, "u32", &Converter.SerialPrimitiveList[Primitive].IsSkinned, 1);
        
        PlatformWriteBinaryToFile(ModelFile, "r32", Converter.SerialPrimitiveList[Primitive].Min.data, 3);
        PlatformWriteBinaryToFile(ModelFile, "r32", Converter.SerialPrimitiveList[Primitive].Max.data, 3);
        
        PlatformWriteBinaryToFile(ModelFile, "m", &Converter.SerialPrimitiveList[Primitive].MaterialName, 1);
        // TODO(Dustin): Do I also want to print the Material Filename?
        
        // The MaterialMapping Table owns the string memory, so don't have to clear it here...
        MstringFree(&Converter.SerialPrimitiveList[Primitive].MaterialFile);
        MstringFree(&Converter.SerialPrimitiveList[Primitive].MaterialName);
    }
    
    PlatformWriteBinaryToFile(ModelFile, "i32", &Converter.MeshIdx, 1);
    
    for (u32 Mesh = 0; Mesh < Converter.MeshIdx; ++Mesh)
    {
        PlatformWriteBinaryToFile(ModelFile, "m", &Converter.SerialMeshList[Mesh].MeshName, 1);
        PlatformWriteBinaryToFile(ModelFile, "u64", &Converter.SerialMeshList[Mesh].PrimitivesCount, 1);
        PlatformWriteBinaryToFile(ModelFile, "i32", Converter.SerialMeshList[Mesh].PrimitivesIdx,
                                  Converter.SerialMeshList[Mesh].PrimitivesCount);
        
        MstringFree(&Converter.SerialMeshList[Mesh].MeshName);
    }
    
    PlatformWriteBinaryToFile(ModelFile, "i32", &Converter.NodeIdx, 1);
    
    for (u32 Node = 0; Node < Converter.NodeIdx; ++Node)
    {
        PlatformWriteBinaryToFile(ModelFile, "m", &Converter.SerialNodeList[Node].Name, 1);
        PlatformWriteBinaryToFile(ModelFile, "i32", &Converter.SerialNodeList[Node].ParentIdx, 1);
        PlatformWriteBinaryToFile(ModelFile, "u64", &Converter.SerialNodeList[Node].ChildrenCount, 1);
        PlatformWriteBinaryToFile(ModelFile, "i32", Converter.SerialNodeList[Node].ChildrenIndices,
                                  Converter.SerialNodeList[Node].ChildrenCount);
        PlatformWriteBinaryToFile(ModelFile, "r32", Converter.SerialNodeList[Node].Translation.data, 3);
        PlatformWriteBinaryToFile(ModelFile, "r32", Converter.SerialNodeList[Node].Scale.data, 3);
        PlatformWriteBinaryToFile(ModelFile, "r32", Converter.SerialNodeList[Node].Rotation.data, 4);
        PlatformWriteBinaryToFile(ModelFile, "i32", &Converter.SerialNodeList[Node].MeshIdx, 1);
        
        MstringFree(&Converter.SerialNodeList[Node].Name);
        MstringFree(&Converter.SerialNodeList[Node].MeshName);
    }
    
    // serialize the disjoint set
    PlatformWriteBinaryToFile(ModelFile, "i32", &Converter.SceneIdx, 1);
    
    for (u32 Scene = 0; Scene < Converter.SceneIdx; ++Scene)
    {
        PlatformWriteBinaryToFile(ModelFile, "u32", &Converter.SerialSceneList[Scene].NodesCount, 1);
        PlatformWriteBinaryToFile(ModelFile, "i32", Converter.SerialSceneList[Scene].NodesIdx,
                                  Converter.SerialSceneList[Scene].NodesCount);
    }
    
    //~ Cleaning up resources
    
    PlatformFlushFile(ModelFile);
    PlatformCloseFile(ModelFile);
    
    PlatformFlushFile(Converter.BinaryDataFile);
    PlatformCloseFile(Converter.BinaryDataFile);
    
    for (i32 i = 0; i < Converter.MaterialNameListIdx; ++i)
    {
        MstringFree(&Converter.MaterialNameList[i]);
    }
    
    MstringFree(&OutputBinaryFilename);
    MstringFree(&OutputModelFilename);
    MstringFree(&Converter.Directory);
    
}
