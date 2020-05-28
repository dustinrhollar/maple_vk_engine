

void InitFrameParams(frame_params *FrameParams)
{
    FrameParams->RenderCommandsCount = 0;
    FrameParams->RenderCommandsCap   = 20;
    FrameParams->RenderCommands      = talloc<render_command>(10);
    FrameParams->GpuCommandsCount    = 0;
    FrameParams->GpuCommandsCap      = 20;
    FrameParams->GpuCommands         = talloc<gpu_command>(FrameParams->GpuCommandsCap);
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
        for (u32 CmdIdx = 0; CmdIdx < FrameParams->RenderCommandsCount; ++CmdIdx)
            RenderCommands[CmdIdx] = FrameParams->RenderCommands[CmdIdx];
        
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
        for (u32 CmdIdx = 0; CmdIdx < FrameParams->GpuCommandsCount; ++CmdIdx)
            GpuCommands[CmdIdx] = FrameParams->GpuCommands[CmdIdx];
        
        // NOTE(Dustin): No need to free the memory since it is a linear allocator...
        FrameParams->GpuCommands = GpuCommands;
    }
    
    FrameParams->GpuCommands[FrameParams->GpuCommandsCount++] = Cmd;
}

void CopyModelAssets(frame_params *FrameParams)
{
    asset *Assets = nullptr;
    u32 Count = 0;
    masset::GetModelAssets(&Assets, &Count);
    
    FrameParams->AssetsCount = Count;
    FrameParams->Assets      = talloc<asset>(Count);
    for (u32 Asset = 0; Asset < Count; ++Asset)
        FrameParams->Assets[Asset] = Assets[Asset];
}
