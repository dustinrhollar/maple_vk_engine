file_internal void RenderAssetMesh(frame_params *FrameParams, mesh *Mesh, mat4 Matrix);
file_internal void RenderAssetNode(frame_params *FrameParams, model_node *Node, mat4 Matrix);
file_internal void RenderAllAssets(frame_params *FrameParams);
file_internal void ProcessKeyboardInput(void *instance, KeyPressEvent event);

void GameStageInit(frame_params* FrameParams)
{
}

void GameStageEntry(frame_params* FrameParams)
{
    RenderAllAssets(FrameParams);
}

void GameStageShutdown(frame_params* FrameParams)
{
}

file_internal void RenderAssetMesh(frame_params *FrameParams, mesh *Mesh, mat4 Matrix)
{
    for (u32 Idx = 0; Idx < Mesh->PrimitivesCount; ++Idx)
    {
        primitive *Primitive = Mesh->Primitives[Idx];
        
        resource_id_t *VBuffers = talloc<resource_id_t>(1);
        u64 *VOffsets = talloc<u64>(1);
        
        VBuffers[0] = Primitive->VertexBuffer;
        VOffsets[0] = 0;
        
        render_draw_command *DrawCommand = talloc<render_draw_command>(1);
        DrawCommand->VertexBuffers      = VBuffers;
        DrawCommand->VertexBuffersCount = 1;
        DrawCommand->Offsets            = VOffsets;
        DrawCommand->IsIndexed          = Primitive->IsIndexed;
        DrawCommand->IndexBuffer        = Primitive->IndexBuffer;
        DrawCommand->Count              = (Primitive->IsIndexed) ? Primitive->IndexCount : Primitive->VertexCount;
        DrawCommand->Material           = Primitive->Material;
        
        object_shader_data ObjectData = {};
        ObjectData.Model = Matrix;
        DrawCommand->ObjectShaderData   = ObjectData;
        
        AddRenderCommand(FrameParams, { RenderCmd_Draw, DrawCommand });
    }
}

file_internal void RenderAssetNode(frame_params *FrameParams, model_node *Node, mat4 Matrix)
{
    mat4 NodeMatrix = mat4(1.0f);
    
    mat4 TranslationMatrix = mat4(1.0f);
    mat4 ScaleMatrix       = mat4(1.0f);
    mat4 RotationMatrix    = mat4(1.0f);
    
    {
        TranslationMatrix = Translate(Node->Translation);
    }
    
    {
        ScaleMatrix = Scale(Node->Scale.x, Node->Scale.y, Node->Scale.z);
    }
    
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
    
    mat4 ModelMatrix = Mul(Matrix, NodeMatrix);
    
    if (Node->Mesh)
    {
        RenderAssetMesh(FrameParams, Node->Mesh, ModelMatrix);
    }
    
    for (u64 i = 0; i < Node->ChildrenCount; ++i)
    {
        RenderAssetNode(FrameParams, Node->Children[i], ModelMatrix);
    }
}

file_internal void RenderAllAssets(frame_params *FrameParams)
{
    for (u32 AssetIdx = 0; AssetIdx < FrameParams->ModelAssetsCount; ++AssetIdx)
    {
        asset Asset = FrameParams->ModelAssets[AssetIdx];
        
        for (i32 DisjointNode = 0; DisjointNode < Asset.Model.RootModelNodesCount; ++DisjointNode)
        {
            model_node *RootNode = Asset.Model.RootModelNodes[DisjointNode];
            RenderAssetNode(FrameParams, RootNode, mat4(1.0f));
        }
    }
}
