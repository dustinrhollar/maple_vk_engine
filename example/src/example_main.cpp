
extern "C" void GameStageEntry(frame_params* FrameParams)
{
#if 1
    FrameParams->GameStageEndTime = 100;
#else
    FrameParams->GameStageEndTime = 500;
#endif
}
