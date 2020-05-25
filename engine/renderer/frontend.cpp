
void RenderStageInit(frame_params *FrameParams)
{
}

void RenderStageEntry(frame_params *FrameParams)
{
    
    for (u32 Cmd = 0; Cmd < FrameParams->RenderCommandsCount; ++Cmd)
    {
        render_command RenderCmd = FrameParams->RenderCommands[Cmd];
        
        switch (RenderCmd.Type)
        {
            case RenderCmd_Draw:
            {
                render_draw_command *Info = static_cast<render_draw_command*>(RenderCmd.Data);
                
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
