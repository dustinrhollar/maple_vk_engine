
void RenderStageInit(frame_params *FrameParams)
{
}

void RenderStageEntry(frame_params *FrameParams)
{
    dyn_uniform_template ObjectDynamicOffset = mresource::GetDynamicObjectUniformTemplate();
    
    // List of draw commands ordered by material. To determine the material of a list,
    // peek the first element to find its material id.
    DynamicArray<DynamicArray<render_draw_command*>> MaterialsToDraw =
        DynamicArray<DynamicArray<render_draw_command*>>(10);
    
    for (u32 Cmd = 0; Cmd < FrameParams->RenderCommandsCount; ++Cmd)
    {
        render_command RenderCmd = FrameParams->RenderCommands[Cmd];
        
        switch (RenderCmd.Type)
        {
            case RenderCmd_Draw:
            {
                render_draw_command *Info = static_cast<render_draw_command*>(RenderCmd.Data);
                
                // Bind Global Descriptors
                // --- Bind Per Material Descriptors
                // --- --- Bind Material Instances Descriptors - NOT SUPPORTED
                // --- --- --- Bind Object Descriptor and any others...
                // --- --- --- Draw
                
                bool Found = false;
                for (u32 FoundMaterial = 0; FoundMaterial < MaterialsToDraw.size; ++FoundMaterial)
                {
                    asset_id_t Existing = MaterialsToDraw[FoundMaterial][0]->Material;
                    asset_id_t New = Info->Material;
                    if ( Existing == New)
                    {
                        Found = true;
                        MaterialsToDraw[FoundMaterial].PushBack(Info);
                    }
                }
                
                if (!Found)
                {
                    DynamicArray<render_draw_command*> NewMaterial = DynamicArray<render_draw_command*>(1);
                    NewMaterial.PushBack(Info);
                    MaterialsToDraw.PushBack(NewMaterial);
                }
            } break;
            
            case RenderCmd_LoadAsset:
            {
            } break;
            
            case RenderCmd_SetViewport:
            {
                render_set_viewport_info *Info = static_cast<render_set_viewport_info*>(RenderCmd.Data);
                
                gpu_set_viewport_info *ViewportInfo = talloc<gpu_set_viewport_info>(1);
                ViewportInfo->Width  = Info->Width;
                ViewportInfo->Height = Info->Height;
                ViewportInfo->X      = Info->X;
                ViewportInfo->Y      = Info->Y;
                AddGpuCommand(FrameParams, { GpuCmd_SetViewport, ViewportInfo });
            } break;
            
            case RenderCmd_SetScissor:
            {
                render_set_scissor_info *Info = static_cast<render_set_scissor_info*>(RenderCmd.Data);
                
                gpu_set_scissor_info *ScissorInfo = talloc<gpu_set_scissor_info>(1);
                ScissorInfo->Extent  = Info->Extent;
                ScissorInfo->XOffset = Info->XOffset;
                ScissorInfo->YOffset = Info->YOffset;
                AddGpuCommand(FrameParams, { GpuCmd_SetScissor, ScissorInfo });
            } break;
            
            case RenderCmd_BindPipeline:
            {
                render_bind_pipeline_info *Info = static_cast<render_bind_pipeline_info*>(RenderCmd.Data);
                
                resource Resource = mresource::GetResource(Info->PipelineId);
                
                gpu_bind_pipeline_info *PipelineBindInfo = talloc<gpu_bind_pipeline_info>(1);
                PipelineBindInfo->Pipeline = Resource.Pipeline.Pipeline;
                
                AddGpuCommand(FrameParams, { GpuCmd_BindPipeline, PipelineBindInfo });
            } break;
            
            case RenderCmd_BindDescriptorSet:
            {
                render_bind_descriptor_set *Info = static_cast<render_bind_descriptor_set*>(RenderCmd.Data);
                
                resource Resource = mresource::GetResource(Info->PipelineId);
                
                gpu_descriptor_set_bind_info *BindInfo = talloc<gpu_descriptor_set_bind_info>(1);
                BindInfo->PipelineLayout      = Resource.Pipeline.Layout;
                BindInfo->DescriptorId        = Info->DescriptorId;
                BindInfo->DynamicOffsets      = Info->DynamicOffsets;
                BindInfo->DynamicOffsetsCount = Info->DynamicOffsetsCount;
                BindInfo->FirstSet            = Info->FirstSet;
                
                AddGpuCommand(FrameParams, { GpuCmd_BindDescriptorSet, BindInfo });
            } break;
            
            case RenderCmd_DrawDevGui:
            {
                render_draw_dev_gui *Info = static_cast<render_draw_dev_gui*>(RenderCmd.Data);
                
                gpu_draw_dev_gui_info *DrawUi = talloc<gpu_draw_dev_gui_info>();
                DrawUi->DevGui = Info->DevGui;
                
                AddGpuCommand(FrameParams, { GpuCmd_DrawDevGui, DrawUi });
            } break;
            
            default:
            {
                mprinte("Unknown render command %d\n", RenderCmd.Type);
            } break;
        }
    }
    
    //~ Now add gpu command for rendering
    
    // Bind Global Data
    for (u32 MaterialIdx = 0; MaterialIdx < MaterialsToDraw.size; ++MaterialIdx)
    {
        // The material for all subsequent objects are the same
        asset_id_t MatId = MaterialsToDraw[MaterialIdx][0]->Material;
        
        asset_material Material = masset::GetAsset(MatId)->Material;
        resource Pipeline = mresource::GetResource(Material.Pipeline);
        
        if (Material.ShaderData.DescriptorSetsCount > GLOBAL_SET &&
            Material.ShaderData.DescriptorSets[GLOBAL_SET].InputDataCount > 0)
        {
            gpu_descriptor_set_bind_info *GlobalBind = talloc<gpu_descriptor_set_bind_info>(1);
            GlobalBind->PipelineLayout      = Pipeline.Pipeline.Layout;
            GlobalBind->DescriptorId        = Material.ShaderData.DescriptorSets[GLOBAL_SET].DescriptorSet;
            GlobalBind->DynamicOffsets      = nullptr;
            GlobalBind->DynamicOffsetsCount = 0;
            GlobalBind->FirstSet            = GLOBAL_SET;
            AddGpuCommand(FrameParams, { GpuCmd_BindDescriptorSet, GlobalBind });
        }
    }
    
    // Bind Material Data and Draw
    for (u32 MaterialIdx = 0; MaterialIdx < MaterialsToDraw.size; ++MaterialIdx)
    {
        // The material for all subsequent objects are the same
        asset_id_t MatId = MaterialsToDraw[MaterialIdx][0]->Material;
        
        asset_material Material = masset::GetAsset(MatId)->Material;
        resource Pipeline = mresource::GetResource(Material.Pipeline);
        
        gpu_bind_pipeline_info *PipelineBind = talloc<gpu_bind_pipeline_info>();
        PipelineBind->Pipeline = Pipeline.Pipeline.Pipeline;
        AddGpuCommand(FrameParams, { GpuCmd_BindPipeline, PipelineBind });
        
        // Bind Descriptor for Material
        if (Material.ShaderData.DescriptorSetsCount > DYNAMIC_SET &&
            Material.ShaderData.DescriptorSets[DYNAMIC_SET].InputDataCount > 0)
        {
            gpu_descriptor_set_bind_info *MaterialBind = talloc<gpu_descriptor_set_bind_info>(1);
            MaterialBind->PipelineLayout      = Pipeline.Pipeline.Layout;
            MaterialBind->DescriptorId        = Material.ShaderData.DescriptorSets[DYNAMIC_SET].DescriptorSet;
            MaterialBind->DynamicOffsets      = nullptr;
            MaterialBind->DynamicOffsetsCount = 0;
            MaterialBind->FirstSet            = DYNAMIC_SET;
            AddGpuCommand(FrameParams, { GpuCmd_BindDescriptorSet, MaterialBind });
        }
        
        u32 OffsetCount = MaterialsToDraw[MaterialIdx].size;
        u32 *ObjOffsets = talloc<u32>(OffsetCount);
        object_shader_data *ObjData = talloc<object_shader_data>(OffsetCount);
        for (u32 Obj = 0; Obj < MaterialsToDraw[MaterialIdx].size; ++Obj)
        {
            // Issue the draw command for the object
            render_draw_command *Info = MaterialsToDraw[MaterialIdx][Obj];
            
            // Get the offsets, and fill in the memory
            ObjOffsets[Obj] = (u32)mresource::DynUniformGetNextOffset(&ObjectDynamicOffset);
            ObjData[Obj]    = Info->ObjectShaderData;
            
            // HACK(Dustin): Assumes that there is only one type of per-object.
            resource DefaultObjectUniform = mresource::GetObjectUniform();
            
            gpu_update_buffer_info *BufferInfo = talloc<gpu_update_buffer_info>(1);
            BufferInfo->Uniform        = DefaultObjectUniform.Id; // HOLD UP!
            BufferInfo->Data           = ObjData + Obj;
            BufferInfo->DataSize       = sizeof(object_shader_data);
            BufferInfo->BufferOffset   = ObjOffsets[Obj];
            AddGpuCommand(FrameParams, { GpuCmd_UpdateBuffer, BufferInfo });
        }
        
        for (u32 Obj = 0; Obj < MaterialsToDraw[MaterialIdx].size; ++Obj)
        {
            // Bind Descriptor for Object
            if (Material.ShaderData.DescriptorSetsCount > STATIC_SET &&
                Material.ShaderData.DescriptorSets[STATIC_SET].InputDataCount > 0)
            {
                gpu_descriptor_set_bind_info *ObjectBind = talloc<gpu_descriptor_set_bind_info>(1);
                ObjectBind->PipelineLayout      = Pipeline.Pipeline.Layout;
                ObjectBind->DescriptorId        = Material.ShaderData.DescriptorSets[STATIC_SET].DescriptorSet;
                ObjectBind->DynamicOffsets      = ObjOffsets + Obj;
                ObjectBind->DynamicOffsetsCount = 1;
                ObjectBind->FirstSet            = STATIC_SET;
                AddGpuCommand(FrameParams, { GpuCmd_BindDescriptorSet, ObjectBind });
            }
            
            // Issue the draw command for the object
            render_draw_command *Info = MaterialsToDraw[MaterialIdx][Obj];
            
            VkBuffer *Buffers = talloc<VkBuffer>(Info->VertexBuffersCount);
            for (u32 Buffer = 0; Buffer < Info->VertexBuffersCount; ++Buffer)
            {
                resource Resource = mresource::GetResource(Info->VertexBuffers[Buffer]);
                Buffers[Buffer] = Resource.VertexBuffer.Buffer.Handle;
            }
            
            resource IResource = mresource::GetResource(Info->IndexBuffer);
            
            gpu_draw_info *DrawInfo = talloc<gpu_draw_info>(1);
            DrawInfo->VertexBuffers      = Buffers;
            DrawInfo->Offsets            = Info->Offsets;
            DrawInfo->VertexBuffersCount = Info->VertexBuffersCount;
            DrawInfo->IsIndexed          = Info->IsIndexed;
            DrawInfo->IndexBuffer        = IResource.IndexBuffer.Buffer.Handle;
            DrawInfo->Count              = Info->Count;
            AddGpuCommand(FrameParams, { GpuCmd_Draw, DrawInfo });
        }
    }
}