
extern "C" void GameStageEntry(frame_params* FrameParams)
{
    for (u32 i = 0; i < FrameParams->AssetsCount; ++i)
    {
        asset Asset = FrameParams->Assets[i];
        if (Asset.Id.Type == Asset_SimpleModel)
        {
            render_command Cmd = { RenderCmd_Draw, Asset.Id };
            FrameParams->RenderCommands[FrameParams->RenderCommandsCount++] = Cmd;
        }
    }
}
