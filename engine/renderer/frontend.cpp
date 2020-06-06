
void RenderStageInit(frame_params *FrameParams)
{
}

void RenderStageEntry(frame_params *FrameParams)
{
    dyn_uniform_template ObjectDynamicOffset = mresource::GetDynamicObjectUniformTemplate();
    
    for (u32 Cmd = 0; Cmd < FrameParams->RenderCommandsCount; ++Cmd)
    {
        render_command RenderCmd = FrameParams->RenderCommands[Cmd];
        
        switch (RenderCmd.Type)
        {
            case RenderCmd_Draw:
            {
                render_draw_command *Info = static_cast<render_draw_command*>(RenderCmd.Data);
                
                // TODO(Dustin): Do a prepass over the objects to collect the offsets into
                // the uniform buffer in order to reduce descriptor offset binds
                
                // HACK(Dustin): Get the hard coded default resources...
                // Will get removed when the material system is in place
                resource DefaultPipelineResource = mresource::GetDefaultPipeline();
                resource DefaultObjectResource   = mresource::GetObjectDescriptorSet();
                resource DefaultObjectUniform    = mresource::GetObjectUniform();
                
                // HACK(Dustin): Bind the Engine Default pipeline...
                gpu_bind_pipeline_info *PipelineBindInfo = talloc<gpu_bind_pipeline_info>(1);
                PipelineBindInfo->Pipeline = DefaultPipelineResource.Pipeline.Pipeline;
                
                AddGpuCommand(FrameParams, { GpuCmd_BindPipeline, PipelineBindInfo });
                
                // Update the descriptor bind for these primitive draws...
                u32 *ObjOffsets = talloc<u32>(1);
                ObjOffsets[0] = (u32)mresource::DynUniformGetNextOffset(&ObjectDynamicOffset);
                
                // NOTE(Dustin): I don't like having to do this....
                object_shader_data *ObjData = talloc<object_shader_data>(1);
                *ObjData = Info->ObjectShaderData;
                
                gpu_update_buffer_info *BufferInfo = talloc<gpu_update_buffer_info>(1);
                BufferInfo->Uniform        = DefaultObjectUniform.Id;
                BufferInfo->Data           = ObjData;
                BufferInfo->DataSize       = sizeof(object_shader_data);
                BufferInfo->BufferOffset   = ObjOffsets[0];
                
                AddGpuCommand(FrameParams, { GpuCmd_UpdateBuffer, BufferInfo });
                
                gpu_descriptor_set_bind_info *BindInfo = talloc<gpu_descriptor_set_bind_info>(1);
                BindInfo->PipelineLayout      = DefaultPipelineResource.Pipeline.Layout;
                BindInfo->DescriptorId        = DefaultObjectResource.Id;
                BindInfo->DynamicOffsets      = ObjOffsets;
                BindInfo->DynamicOffsetsCount = 1;
                BindInfo->FirstSet            = STATIC_SET; // set number of per-object set
                
                AddGpuCommand(FrameParams, { GpuCmd_BindDescriptorSet, BindInfo });
                
                // TODO(Dustin): Place primitives in a list to draw with similar materials...
                for (int i = 0; i < Info->PrimitivesCount; ++i)
                {
                    primitive Primitive = Info->PrimitivesToDraw[i];
                    
                    VkBuffer VertexBuffer = mresource::GetResource(Primitive.VertexBuffer).VertexBuffer.Buffer.Handle;
                    VkBuffer IndexBuffer  = mresource::GetResource(Primitive.IndexBuffer).IndexBuffer.Buffer.Handle;
                    
                    VkBuffer *Buffers       = talloc<VkBuffer>(1);
                    Buffers[0]              = VertexBuffer;
                    
                    u64 *Offsets            = talloc<u64>(1);
                    Offsets[0]              = 0;
                    
                    gpu_draw_info *DrawInfo = talloc<gpu_draw_info>(1);
                    DrawInfo->VertexBuffers      = Buffers;
                    DrawInfo->Offsets            = Offsets;
                    DrawInfo->VertexBuffersCount = 1;
                    DrawInfo->IsIndexed          = Primitive.IsIndexed;
                    DrawInfo->IndexBuffer        = IndexBuffer;
                    DrawInfo->Count              = (Primitive.IsIndexed) ? Primitive.IndexCount : Primitive.VertexCount;
                    
                    AddGpuCommand(FrameParams, { GpuCmd_Draw, DrawInfo });
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
            
            default:
            {
                mprinte("Unknown render command %d\n", RenderCmd.Type);
            } break;
        }
    }
}
