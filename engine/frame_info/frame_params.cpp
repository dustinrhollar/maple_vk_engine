

void InitFrameParams(frame_params *FrameParams)
{
    FrameParams->RenderCommands      = talloc<render_command>(10);
    FrameParams->RenderCommandsCount = 0;
    FrameParams->RenderCommandsCap   = 10;
    FrameParams->GpuCommands         = talloc<gpu_command>(10);
    FrameParams->GpuCommandsCount    = 0;
    FrameParams->GpuCommandsCap      = 10;
}

void FreeFrameParams(frame_params *FrameParams)
{
    pfree(FrameParams->RenderCommands);
    pfree(FrameParams->GpuCommands);
    FrameParams->RenderCommandsCount = 0;
    FrameParams->RenderCommandsCap   = 0;
    FrameParams->GpuCommandsCount    = 0;
    FrameParams->GpuCommandsCap      = 0;
}

void AddRenderCommand(frame_params *FrameParams, render_command Cmd)
{
    if (FrameParams->RenderCommandsCount + 1 >= FrameParams->RenderCommandsCap)
    { //  resize
        FrameParams->RenderCommandsCap *= 2;
        render_command *RenderCommands = talloc<render_command>(FrameParams->RenderCommandsCap);
        for (u32 Cmd = 0; Cmd < FrameParams->RenderCommandsCount; ++Cmd)
            RenderCommands[Cmd] = FrameParams->RenderCommands[Cmd];
        
        // NOTE(Dustin): No need to free the memory since it is a linear allocator...
        FrameParams->RenderCommands = RenderCommands;
    }
    
    FrameParams->RenderCommands[FrameParams->RenderCommandsCount++] = Cmd;
}

void AddGpuCommand(frame_params *FrameParams, gpu_command Cmd)
{
    if (FrameParams->GpuCommandsCount + 1 >= FrameParams->GpuCommandsCap)
    { //  resize
        FrameParams->GpuCommandsCap *= 2;
        gpu_command *GpuCommands = talloc<gpu_command>(FrameParams->GpuCommandsCap);
        for (u32 Cmd = 0; Cmd < FrameParams->GpuCommandsCount; ++Cmd)
            GpuCommands[Cmd] = FrameParams->GpuCommands[Cmd];
        
        // NOTE(Dustin): No need to free the memory since it is a linear allocator...
        FrameParams->GpuCommands = GpuCommands;
    }
    
    FrameParams->GpuCommands[FrameParams->GpuCommandsCount++] = Cmd;
}
