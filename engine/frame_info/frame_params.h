#ifndef ENGINE_FRAME_INFO_FRAME_PARAMS_H
#define ENGINE_FRAME_INFO_FRAME_PARAMS_H

struct frame_params
{
    u64             Frame;
    
    //~ Timing
    u64             FrameStartTime;
    
    u64             GameStageEndTime;
    
    u64             RenderStageStartTime;
    u64             RenderStageEndTime;
    
    //~ Resources
    resource_t     *Resources;
    u32             ResourcesCount;
};

#endif //ENGINE_FRAME_INFO_FRAME_PARAMS_H
