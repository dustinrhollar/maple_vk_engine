
namespace masset
{
    file_internal i32 ConvertNode(mesh_converter *Converter, cgltf_node *CgNode);
    file_internal i32 ConvertScene(mesh_converter *Converter, cgltf_scene *CgScene);
    
    file_internal i32 ConvertPrimitive(mesh_converter *Converter, cgltf_primitive *CgPrimitive)
    {
        primitive_serial Primitive = {};
        Primitive.PrimitiveIdx = Converter->PrimitiveIdx++;
        
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
                    for (int i = 0; i < Primitive.IndexCount; ++i)
                    {
                        indices[i] = (u32)indices_buffer[i];
                    }
                } break;
                
                case cgltf_component_type_r_8u:
                {
                    u8 *indices_buffer = (u8*)unknown_indices_buffer;
                    
                    for (int i = 0; i < Primitive.IndexCount; ++i)
                    {
                        indices[i] = (u32)indices_buffer[i];
                    }
                } break;
                
                case cgltf_component_type_r_32u:
                {
                    u32 *indices_buffer = (u32*)unknown_indices_buffer;
                    
                    for (int i = 0; i < Primitive.IndexCount; ++i)
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
            Node.Rotation = {0.0f,0.0f,0.0f,0.0f};
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
        
        return Node.NodeIdx;
    }
    
    file_internal i32 ConvertScene(mesh_converter *Converter, cgltf_scene *CgScene)
    {
        model_serial Model = {};
        Model.ModelIdx     = Converter->ModelIdx++;
        Model.NodesCount   = (u32)CgScene->nodes_count;
        Model.NodesIdx     = talloc<i32>(Model.NodesCount);
        
        for (int i = 0; i < Model.NodesCount; ++i)
        {
            i32 Idx = ConvertNode(Converter, CgScene->nodes[i]);
            Model.NodesIdx[i] = Idx;
        }
        
        Converter->SerialModelList[Model.ModelIdx] = Model;
        
        return Model.ModelIdx;
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
            }
        }
        
        ptr++;
        
        size_t len = ptr - cfile;
        char *dirname = (char*)talloc(len);
        strncpy(dirname, cfile, len);
        
        Converter.Directory = InitJString(dirname, (u32)len);
        
        PlatformPrintMessage(EConsoleColor::Yellow, EConsoleColor::DarkGrey, "Filename %s\n", cfile);
        PlatformPrintMessage(EConsoleColor::Yellow, EConsoleColor::DarkGrey, "Directory %s\n", Converter.Directory.GetCStr());
        
        cgltf_options options = {0};
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
        
        //~ Convert the Scene
        
        Converter.NodeIdx         = 0;
        Converter.MeshIdx         = 0;
        Converter.PrimitiveIdx    = 0;
        Converter.SerialNodeList  = talloc<model_node_serial>(Converter.Data->nodes_count);
        Converter.SerialModelList = talloc<model_serial>(Converter.Data->scenes_count);
        Converter.SerialMeshList  = talloc<mesh_serial>(Converter.Data->meshes_count);
        CreateFileBuffer(&Converter.PrimitiveDataBlock);
        
        // Determine the amount of primitives in the file
        u32 PrimitivesCount = 0;
        for (u32 Mesh = 0; Mesh < Converter.Data->meshes_count; ++Mesh)
        {
            PrimitivesCount += Converter.Data->meshes[Mesh].primitives_count;
        }
        
        Converter.SerialPrimitiveList = talloc<primitive_serial>(PrimitivesCount);
        
        ConvertScene(&Converter, &Converter.Data->scenes[0]);
        
        //~ Serialize the mesh
        
        u32 ExpectedFileSize = sizeof(model_node_serial) * Converter.NodeIdx + sizeof(u32);
        ExpectedFileSize    += sizeof(primitive_serial) * Converter.PrimitiveIdx + sizeof(u32);
        ExpectedFileSize    += sizeof(mesh_serial) * Converter.MeshIdx + sizeof(u32);
        
        FileBuffer Buffer;
        CreateFileBuffer(&Buffer, ExpectedFileSize);
        
        // Serialize the binary buffer
        u64 BufferSize = Converter.PrimitiveDataBlock.brkp - Converter.PrimitiveDataBlock.start;
        UInt64ToBinaryBuffer(&Buffer, &BufferSize, 1);
        CharToBinaryBuffer(&Buffer, Converter.PrimitiveDataBlock.start, BufferSize);
        
        // Serialize primitives
        Int32ToBinaryBuffer(&Buffer, &Converter.PrimitiveIdx, 1);
        
        for (u32 Primitive = 0; Primitive < Converter.PrimitiveIdx; ++Primitive)
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
        }
        
        // Serialize the meshes
        Int32ToBinaryBuffer(&Buffer, &Converter.MeshIdx, 1);
        
        for (u32 Mesh = 0; Mesh < Converter.MeshIdx; ++Mesh)
        {
            JStringToBinaryBuffer(&Buffer, Converter.SerialMeshList[Mesh].MeshName);
            
            UInt64ToBinaryBuffer(&Buffer, &Converter.SerialMeshList[Mesh].PrimitivesCount, 1);
            Int32ToBinaryBuffer(&Buffer, Converter.SerialMeshList[Mesh].PrimitivesIdx,
                                Converter.SerialMeshList[Mesh].PrimitivesCount);
            
            Converter.SerialMeshList[Mesh].MeshName.Clear();
        }
        
        // Serialize the node heirarchy
        Int32ToBinaryBuffer(&Buffer, &Converter.NodeIdx, 1);
        
        for (u32 Node = 0; Node < Converter.NodeIdx; ++Node)
        {
            JStringToBinaryBuffer(&Buffer, Converter.SerialNodeList[Node].Name);
            JStringToBinaryBuffer(&Buffer, Converter.SerialNodeList[Node].MeshName);
            
            Int32ToBinaryBuffer(&Buffer, &Converter.SerialNodeList[Node].ParentIdx, 1);
            
            UInt64ToBinaryBuffer(&Buffer, &Converter.SerialNodeList[Node].ChildrenCount, 1);
            Int32ToBinaryBuffer(&Buffer, Converter.SerialNodeList[Node].ChildrenIndices,
                                Converter.SerialNodeList[Node].ChildrenCount);
            
            FloatToBinaryBuffer(&Buffer, Converter.SerialNodeList[Node].Translation.data, 3);
            FloatToBinaryBuffer(&Buffer, Converter.SerialNodeList[Node].Scale.data, 3);
            FloatToBinaryBuffer(&Buffer, Converter.SerialNodeList[Node].Rotation.data, 4);
            
            Converter.SerialNodeList[Node].Name.Clear();
            Converter.SerialNodeList[Node].MeshName.Clear();
        }
        
        // Finally...write to the file
        jstring BinaryFilename = InitJString("fox.model");
        
        PlatformWriteBufferToFile(BinaryFilename, Buffer.start, Buffer.brkp - Buffer.start);
        
        // Clean Resources
        DestroyFileBuffer(&Converter.PrimitiveDataBlock);
        DestroyFileBuffer(&Buffer);
        BinaryFilename.Clear();
        Converter.Directory.Clear();
        
#if 0
        //~ As a test, let's read the file back in to see if it wrote correctly...
        jstring FileContents = PlatformLoadFile(File);
        Buffer.cap   = FileContents.len;
        Buffer.start = FileContents.GetCStr();
        Buffer.brkp  = Buffer.start;
        
        i32 ReadNodeCount;
        ReadInt32FromBinaryBuffer(&Buffer, &ReadNodeCount);
        assert(ReadNodeCount == Converter.NodeIdx);
        
        model_node_serial *ReadNodeList = talloc<model_node_serial>(ReadNodeCount);
        for (u32 Node = 0; Node < ReadNodeCount; ++Node)
        {
            ReadJStringFromBinaryBuffer(&Buffer, &ReadNodeList[Node].Name);
            ReadJStringFromBinaryBuffer(&Buffer, &ReadNodeList[Node].MeshName);
            
            ReadInt32FromBinaryBuffer(&Buffer, &ReadNodeList[Node].ParentIdx);
            
            ReadUInt64FromBinaryBuffer(&Buffer, &ReadNodeList[Node].ChildrenCount);
            ReadNodeList[Node].ChildrenIndices = talloc<i32>(ReadNodeList[Node].ChildrenCount);
            
            for (i32 Idx = 0; Idx < ReadNodeList[Node].ChildrenCount; ++Idx)
            {
                ReadInt32FromBinaryBuffer(&Buffer, &ReadNodeList[Node].ChildrenIndices[Idx]);
            }
            
            // Tranlation vec
            ReadFloatFromBinaryBuffer(&Buffer, &ReadNodeList[Node].Translation.x);
            ReadFloatFromBinaryBuffer(&Buffer, &ReadNodeList[Node].Translation.y);
            ReadFloatFromBinaryBuffer(&Buffer, &ReadNodeList[Node].Translation.z);
            
            // Scaling
            ReadFloatFromBinaryBuffer(&Buffer, &ReadNodeList[Node].Scale.x);
            ReadFloatFromBinaryBuffer(&Buffer, &ReadNodeList[Node].Scale.y);
            ReadFloatFromBinaryBuffer(&Buffer, &ReadNodeList[Node].Scale.z);
            
            // Quaternion Rotation
            ReadFloatFromBinaryBuffer(&Buffer, &ReadNodeList[Node].Rotation.x);
            ReadFloatFromBinaryBuffer(&Buffer, &ReadNodeList[Node].Rotation.y);
            ReadFloatFromBinaryBuffer(&Buffer, &ReadNodeList[Node].Rotation.z);
            ReadFloatFromBinaryBuffer(&Buffer, &ReadNodeList[Node].Rotation.w);
        }
        
        FileContents.Clear();
        File.Clear();
        
        //~ As a test, let's read the mesh file back in to see if it wrote correctly...
        
        FileContents = PlatformLoadFile(File);
        Buffer.cap   = FileContents.len;
        Buffer.start = FileContents.GetCStr();
        Buffer.brkp  = Buffer.start;
        
        i32 ReadMeshCount;
        ReadInt32FromBinaryBuffer(&Buffer, &ReadMeshCount);
        assert(ReadMeshCount == Converter.MeshIdx);
        
        mesh_serial *ReadMeshList = talloc<mesh_serial>(ReadMeshCount);
        for (u32 Mesh = 0; Mesh < ReadMeshCount; ++Mesh)
        {
            ReadJStringFromBinaryBuffer(&Buffer, &ReadMeshList[Mesh].MeshName);
            
            ReadUInt64FromBinaryBuffer(&Buffer, &ReadMeshList[Mesh].PrimitivesCount);
            ReadMeshList[Mesh].PrimitivesIdx = talloc<i32>(ReadMeshList[Mesh].PrimitivesCount);
            
            for (i32 Idx = 0; Idx < ReadMeshList[Mesh].PrimitivesCount; ++Idx)
            {
                ReadInt32FromBinaryBuffer(&Buffer, &ReadMeshList[Mesh].PrimitivesIdx[Idx]);
            }
        }
        
        FileContents.Clear();
        File.Clear();
        
        //~ As a test, let's read the mesh file back in to see if it wrote correctly...
        
        FileContents = PlatformLoadFile(File);
        Buffer.cap   = FileContents.len;
        Buffer.start = FileContents.GetCStr();
        Buffer.brkp  = Buffer.start;
        
        
        i32 ReadPrimitiveCount;
        ReadInt32FromBinaryBuffer(&Buffer, &ReadPrimitiveCount);
        assert(ReadPrimitiveCount == Converter.PrimitiveIdx);
        
        primitive_serial *ReadPrimitiveList = talloc<primitive_serial>(ReadPrimitiveCount);
        for (u32 Primitive = 0; Primitive < ReadPrimitiveCount; ++Primitive)
        {
            //ReadJStringFromBinaryBuffer(&Buffer, &ReadPrimitiveList[Primitive].BinaryFilename);
            
            // Data info
            ReadUInt64FromBinaryBuffer(&Buffer, &ReadPrimitiveList[Primitive].Offset);
            ReadUInt32FromBinaryBuffer(&Buffer, &ReadPrimitiveList[Primitive].IndexCount);
            ReadUInt32FromBinaryBuffer(&Buffer, &ReadPrimitiveList[Primitive].VertexCount);
            ReadBoolFromBinaryBuffer(&Buffer, &ReadPrimitiveList[Primitive].IsSkinned);
            
            
            // Min/Max data for the primitve
            ReadFloatFromBinaryBuffer(&Buffer, &ReadPrimitiveList[Primitive].Min.x);
            ReadFloatFromBinaryBuffer(&Buffer, &ReadPrimitiveList[Primitive].Min.y);
            ReadFloatFromBinaryBuffer(&Buffer, &ReadPrimitiveList[Primitive].Min.z);
            
            ReadFloatFromBinaryBuffer(&Buffer, &ReadPrimitiveList[Primitive].Max.x);
            ReadFloatFromBinaryBuffer(&Buffer, &ReadPrimitiveList[Primitive].Max.y);
            ReadFloatFromBinaryBuffer(&Buffer, &ReadPrimitiveList[Primitive].Max.z);
            
            // Vertex/Index Stride
            ReadUInt32FromBinaryBuffer(&Buffer, &ReadPrimitiveList[Primitive].VertexStride);
            ReadUInt32FromBinaryBuffer(&Buffer, &ReadPrimitiveList[Primitive].IndexStride);
        }
        
        FileContents.Clear();
        File.Clear();
#endif
    }
    
} // masset