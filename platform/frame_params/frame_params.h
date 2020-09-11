#ifndef PLATFORM_FRAME_PARAMS_H
#define PLATFORM_FRAME_PARAMS_H

typedef struct frame_params 
{
    //~ Timing
    u64             Frame;
    
    u64             FrameStartTime;
    
    u64             GameStageEndTime;
    
    u64             RenderStageStartTime;
    u64             RenderStageEndTime;
    
    //~ Input
    
    input           Input;
    
    //~ Graphics
    
    struct graphics_api    *Graphics;
    struct platform        *Platform;
    struct mp_command_list *CommandList;
    
    //~ Scene
    
    struct camera          *Camera;
    struct world           *World;
    
} frame_params;

#endif //PLATFORM_FRAME_PARAMS_H
