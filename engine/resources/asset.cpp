namespace masset
{
    // TODO(Dustin): After memory manager has been refactored, have a pool here for
    // model memory rather than just allocating from global storage.
    
    file_global const u32  MaxAssets = 10;
    file_global asset      AssetRegistry[MaxAssets];
    file_global asset     *AssetModelRegistry[MaxAssets];
    file_global asset_id_t NextAssetId = 0;
    
    
    //~  Model Asset Functionality
    
    struct mesh_loader
    {
        // NOTE(Dustin): THIS IS TEMPORARY UNTIL STAGES HAVE BEEN SPLIT
        VkCommandPool CommandPool;
        
        cgltf_data *Data;
        
        jstring Filename;
        jstring Directory;
    };
    
    //void ParseTexture(mesh_loader *Loader, cgltf_texture *Texture, texture_parameters *TexParam);
    //void ParseMaterial(mesh_loader *Loader, cgltf_material *cg_material, material_parameters *Material);
    file_internal render_component CreateRenderComponent(VkCommandPool CommandPool,
                                                         size_t vertex_count, size_t vertex_stride, void *vertices,
                                                         size_t index_count, size_t index_stride, void *indices);
    file_internal void ParsePrimitive(mesh_loader *Loader, cgltf_primitive *cg_primitive, primitive *Primitive);
    file_internal void ParseMesh(mesh_loader *Loader, cgltf_mesh *CgMesh, mesh *Mesh);
    file_internal void ParseNode(mesh_loader *Loader, cgltf_node *CgNode, model_node  *Node);
    file_internal void ParseScene(mesh_loader *Loader, cgltf_scene *CgScene, model *Model);
    
    file_internal void RenderModelAsset(asset *Asset);
    file_internal void RenderModelMesh(mesh *Mesh, mat4 Matrix);
    file_internal void RenderModelNode(model_node *Node, mat4 Matrix);
    
    file_internal render_component CreateRenderComponent(VkCommandPool CommandPool,
                                                         size_t vertex_count, size_t vertex_stride, void *vertices,
                                                         size_t index_count, size_t index_stride, void *indices)
    {
        
        render_component rcomp = {};
        
        // Create vertex info
        VkBufferCreateInfo vertex_buffer_info = {};
        vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertex_buffer_info.size = vertex_count * vertex_stride;
        vertex_buffer_info.usage =
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        
        VmaAllocationCreateInfo vertex_alloc_info = {};
        vertex_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        
        vk::CreateVmaBufferWithStaging(CommandPool,
                                       vertex_buffer_info,
                                       vertex_alloc_info,
                                       rcomp.VertexBuffer.Handle,
                                       rcomp.VertexBuffer.Memory,
                                       vertices,
                                       vertex_count * vertex_stride);
        
        rcomp.VertexBuffer.Size = vertex_count * vertex_stride;
        
        if (index_count > 0)
        {
            rcomp.IndexedDraw = true;
            // Create index info
            VkBufferCreateInfo index_buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            index_buffer_info.size = index_count * index_stride;
            index_buffer_info.usage =
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            
            VmaAllocationCreateInfo index_alloc_info = {};
            index_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            
            vk::CreateVmaBufferWithStaging(CommandPool,
                                           index_buffer_info,
                                           index_alloc_info,
                                           rcomp.IndexBuffer.Handle,
                                           rcomp.IndexBuffer.Memory,
                                           indices,
                                           index_count * index_stride);
            
            rcomp.IndexBuffer.Size = index_count * index_stride;
            
            rcomp.Count = (u32)index_count;
        }
        else
        {
            rcomp.IndexedDraw = false;
            rcomp.Count = (u32)vertex_count;
        }
        
        return rcomp;
    };
    
    void DestroyRenderComponent(render_component rcom) {
        vk::DestroyVmaBuffer(rcom.VertexBuffer.Handle, rcom.VertexBuffer.Memory);
        vk::DestroyVmaBuffer(rcom.IndexBuffer.Handle, rcom.IndexBuffer.Memory);
    }
    
    file_internal void ParsePrimitive(mesh_loader *Loader, cgltf_primitive *CgPrimitive, primitive *Primitive)
    {
        
        /* Create a new Material Entity ???
        // parse the material
        if (CgPrimitive->material) {
            MaterialParameters mat_param = {};
            ParseMaterial(CgPrimitive->material, &mat_param);
            
            Pritimive->MaterialId = CreateMaterial(mat_param);
        }
        */
        
        if (CgPrimitive->indices)
            Primitive->IndexCount = CgPrimitive->indices->count;
        else
            Primitive->IndexCount = 0;
        
        // figure out the vertex count
        for (size_t k = 0; k < CgPrimitive->attributes_count; ++k)
        { // find the position attribute
            cgltf_attribute attrib = CgPrimitive->attributes[k];
            
            if (attrib.type == cgltf_attribute_type_position)
            {
                cgltf_accessor *accessor = attrib.data;
                
                Primitive->VertexCount = attrib.data->count;
                
                // TODO(Dustin): If the Min/Max is not provided need to manually
                // track the min/max points while copying over the vertex data.
                if (accessor->has_min)
                {
                    Primitive->Min = MakeVec3(accessor->min);
                }
                
                if (accessor->has_max)
                {
                    Primitive->Max = MakeVec3(accessor->max);
                }
                
                break;
            }
        }
        
        
        size_t data_size = sizeof(u32) * Primitive->IndexCount + sizeof(Vertex) * Primitive->VertexCount;
        
        Primitive->IndicesOffset = 0;
        Primitive->VerticesOffeset = sizeof(u32) * Primitive->IndexCount;
        
        Primitive->DataBlock = (char*)palloc(data_size);
        
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
                    for (int i = 0; i < Primitive->VertexCount; ++i)
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
                    for (int i = 0; i < Primitive->VertexCount; ++i)
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
        if (Primitive->IndexCount > 0)
        {
            indices = (u32*)Primitive->DataBlock + Primitive->IndicesOffset;
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
                    for (int i = 0; i < Primitive->IndexCount; ++i)
                    {
                        indices[i] = (u32)indices_buffer[i];
                    }
                } break;
                
                case cgltf_component_type_r_8u:
                {
                    u8 *indices_buffer = (u8*)unknown_indices_buffer;
                    
                    for (int i = 0; i < Primitive->IndexCount; ++i)
                    {
                        indices[i] = (u32)indices_buffer[i];
                    }
                } break;
                
                case cgltf_component_type_r_32u:
                {
                    u32 *indices_buffer = (u32*)unknown_indices_buffer;
                    
                    for (int i = 0; i < Primitive->IndexCount; ++i)
                    {
                        indices[i] = (u32)indices_buffer[i];
                    }
                } break;
                
                default:
                { /* TODO(Dustin): Might want to Log the invalid index size */ } break;
            }
            
#if 0
            // TEST CODE
            u16 *iter = (u16*)((char*)Primitive->DataBlock + Primitive->IndicesOffset);
            for (int i = 0; i < Primitive->IndexCount; i += 3)
            {
                printf("%d %d %d\n", *(iter+0), *(iter+1), *(iter+3));
                
                iter += 3;
            }
            printf("----------------------------------------\n");
#endif
        }
        
        
        // Copy the vertex data over to its memory block
        {
            Vertex *vertices = (Vertex*)((char*)Primitive->DataBlock + sizeof(u32) * Primitive->IndexCount);
            
            vec2 dummy_vec2 = { 0.0f, 0.0f };
            vec3 dummy_vec3 = { 0.f, 0.0f, 0.0f };
            vec4 dummy_vec4 = { 0.5f, 0.5f, 0.5f, 1.0f };
            
            for (size_t idx = 0; idx < Primitive->VertexCount; ++idx)
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
        
        u64 render_comp_stride = sizeof(Vertex);
        Primitive->RenderComp = CreateRenderComponent(Loader->CommandPool,
                                                      Primitive->VertexCount,
                                                      render_comp_stride,
                                                      ((char*)Primitive->DataBlock + sizeof(u32) * Primitive->IndexCount),
                                                      Primitive->IndexCount,
                                                      sizeof(u32),
                                                      indices);
    }
    
    file_internal void ParseMesh(mesh_loader *Loader, cgltf_mesh *CgMesh, mesh *Mesh)
    {
        Mesh->Entity      = ecs::CreateEntity();
        
        Mesh->PrimitivesCount = CgMesh->primitives_count;
        Mesh->Primitives      = palloc<primitive>(Mesh->PrimitivesCount);
        
        for (int i = 0; i < CgMesh->primitives_count; ++i)
        {
            ParsePrimitive(Loader, CgMesh->primitives + i, Mesh->Primitives + i);
        }
    }
    
    file_internal void ParseNode(mesh_loader *Loader, cgltf_node *CgNode, model_node *Node)
    {
        if (CgNode->name)
        {
            Node->Name = pstring(CgNode->name);
        }
        
        if (CgNode->has_translation)
        {
            Node->HasTranslation = true;
            Node->Translation = MakeVec3(CgNode->translation);
        }
        else
        {
            Node->HasTranslation = false;
            Node->Translation = {0.0f,0.0f,0.0f};
        }
        
        if (CgNode->has_scale)
        {
            Node->HasScale = true;
            Node->Scale = MakeVec3(CgNode->scale);
        }
        else
        {
            Node->HasScale = false;
            Node->Scale = {1.0f,1.0f,1.0f};
        }
        
        if (CgNode->has_rotation)
        {
            Node->HasRotation = true;
            Node->Rotation = MakeVec4(CgNode->rotation);
        }
        else
        {
            Node->HasRotation = false;
            Node->Rotation = {0.0f,0.0f,0.0f,0.0f};
        }
        
        if (CgNode->has_matrix)
        {
            Node->HasMatrix = true;
            Node->Matrix = MakeMat4(CgNode->matrix);
        }
        else
        {
            Node->HasMatrix = false;
            Node->Matrix = mat4(1.0f);
        }
        
        // Pointer to a mesh, if one exists
        if (CgNode->mesh)
        {
            cgltf_mesh *cg_mesh = CgNode->mesh;
            
            Node->Mesh = palloc<mesh>(1);
            ParseMesh(Loader, cg_mesh, Node->Mesh);
        }
        else
        {
            Node->Mesh = nullptr;
        }
        
        // Recurse down the set to parse all child nodes
        Node->ChildrenCount = CgNode->children_count;
        Node->Children = palloc<model_node>(Node->ChildrenCount);
        
        for (int i = 0; i < CgNode->children_count; ++i)
        {
            cgltf_node *ChildCgNode = CgNode->children[i];
            
            Node->Children[i] = {};
            ParseNode(Loader, ChildCgNode, Node->Children + i);
        }
    }
    
    file_internal void ParseScene(mesh_loader *Loader, cgltf_scene *CgScene, asset_model *ModelAsset)
    {
        // Initialize model  memory
        ModelAsset->Model.NodesCount = (u32)CgScene->nodes_count;
        ModelAsset->Model.Nodes      = palloc<model_node>(ModelAsset->Model.NodesCount);
        
        // parse each disjoint set in this model
        for (int i = 0; i < CgScene->nodes_count; ++i)
        {
            *(ModelAsset->Model.Nodes + i) = {};
            (ModelAsset->Model.Nodes + i)->Parent = nullptr;
            
            // Parse the node and its descendents
            ParseNode(Loader, CgScene->nodes[i], ModelAsset->Model.Nodes + i);
        }
    }
    
    asset_id_t LoadModel(jstring Filename, VkCommandPool CommandPool)
    {
        // TODO(Dustin): In the future, determine the file extension, if the extension
        // is not ".model" format, then do glTF loading/conversion on the model
        mesh_loader Loader = {};
        Loader.Filename = Filename;
        Loader.CommandPool = CommandPool;
        
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
        
        Loader.Directory = InitJString(dirname, (u32)len);
        
        PlatformPrintMessage(EConsoleColor::Yellow, EConsoleColor::DarkGrey, "Filename %s\n", cfile);
        PlatformPrintMessage(EConsoleColor::Yellow, EConsoleColor::DarkGrey, "Directory %s\n", Loader.Directory.GetCStr());
        
        cgltf_options options = {0};
        Loader.Data = nullptr;
        cgltf_result result = cgltf_parse_file(&options, Loader.Filename.GetCStr(), &Loader.Data);
        if (result != cgltf_result_success)
        {
            mprinte("Failed to load mesh %s!\n", Filename.GetCStr());
            return -1;
        }
        
        // load the buffers
        // NOTE(Dustin): Maybe in the future, stream the data directly to the gpu?
        result = cgltf_load_buffers(&options, Loader.Data, Filename.GetCStr());
        if (result != cgltf_result_success)
        {
            mprinte("Failed to load mesh buffers %s!\n", Filename.GetCStr());
            return -1;
        }
        
        if (NextAssetId + 1 >= MaxAssets)
        {
            mprinte("Asset registry is full!\n");
            return -1;
        }
        
        asset *ModelAsset = &AssetRegistry[NextAssetId++];
        ModelAsset->Type = Asset_Model;
        ModelAsset->Id   = NextAssetId - 1;
        
        // NOTE(Dustin): Hard limit on the scenes imported from
        // a glTF file.
        assert(Loader.Data->scenes_count == 1 && "glTF File being loaded has more than one scene!");
        ParseScene(&Loader, &Loader.Data->scenes[0], &ModelAsset->Model);
        
        // TODO(Dustin): Animations, Textures (?), and Materials (?)...
        
        cgltf_free(Loader.Data);
        Loader.Directory.Clear();
        
        return ModelAsset->Id;
    }
    
    
    //~ Material Asset Functionality
    
    
    
    //~ Texture Asset Functionality
    
    
    //~ Render Command
    
    file_internal void RenderModelMesh(mesh *Mesh, mat4 Matrix)
    {
        
    }
    
    file_internal void RenderModelNode(model_node *Node, mat4 Matrix)
    {
        mat4 NodeMatrix = mat4(1.0f);
        
        if (Node->HasMatrix)
        {
            NodeMatrix = Node->Matrix;
        }
        else
        {
            mat4 TranslationMatrix = mat4(1.0f);
            mat4 ScaleMatrix       = mat4(1.0f);
            mat4 RotationMatrix    = mat4(1.0f);
            
            if (Node->HasTranslation)
            {
                TranslationMatrix = Translate(Node->Translation);
                
            }
            
            if (Node->HasScale)
            {
                ScaleMatrix = Scale(Node->Scale.x, Node->Scale.y, Node->Scale.z);
            }
            
            if (Node->HasRotation)
            {
                vec3 axis = Node->Rotation.xyz;
                float theta = Node->Rotation.w;
                
                Quaternion rotation = MakeQuaternion(axis.x,axis.y,axis.z,theta);
                RotationMatrix = GetQuaternionRotationMatrix(rotation);
            }
            
            // Multiplication order = T * R * S
            NodeMatrix = Mul(NodeMatrix, ScaleMatrix);
            NodeMatrix = Mul(NodeMatrix, RotationMatrix);
            NodeMatrix = Mul(NodeMatrix, TranslationMatrix);
        }
        
        mat4 ModelMatrix = Mul(Matrix, NodeMatrix);
        
        if (Node->Mesh)
        {
            RenderModelMesh(Node->Mesh, ModelMatrix);
        }
        
        for (int i = 0; i < Node->ChildrenCount; ++i)
        {
            RenderModelNode(Node->Children + i, ModelMatrix);
        }
        
    }
    
    file_internal void RenderModelAsset(asset *Asset)
    {
        for (u32 DisjointNode = 0; DisjointNode < Asset->Model.Model.NodesCount; ++DisjointNode)
        {
            model_node *RootNode = Asset->Model.Model.Nodes + DisjointNode;
            RenderModelNode(RootNode, mat4(1.0f));
        }
    }
    
    void Render(asset_id_t AssetId)
    {
        asset *Asset = &AssetRegistry[AssetId];
        assert(Asset && "Attempted to retrieve an asset but it was null!");
        
        switch (Asset->Type)
        {
            case Asset_Model:
            {
                RenderModelAsset(Asset);
            } break;
            
            case Asset_Texture:
            case Asset_Material:
            default: break;
        }
    }
}; // masset