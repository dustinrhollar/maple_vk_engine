#ifndef ENGINE_FRAME_INFO_FRAME_PARAMS_H
#define ENGINE_FRAME_INFO_FRAME_PARAMS_H

struct render_command;
struct gpu_command;
struct asset;

struct frame_params
{
    u64             Frame;
    
    //~ Timing
    u64             FrameStartTime;
    
    u64             GameStageEndTime;
    
    u64             RenderStageStartTime;
    u64             RenderStageEndTime;
    
    u64             GpuStageStartTime;
    u64             GpuStageEndTime;
    
    //~ Resources
    asset          *Assets;
    u32             AssetsCount;
    
    //~ Commands
    // NOTE(Dustin): Need to determine the internal data structure of these
    // command lists... Since the frame_params uses a tagged heap for memory
    // allocation, a resizable array could end up wasting a lot of memory...
    // since the old array's memory will not be recycled.
    render_command *RenderCommands;
    u32             RenderCommandsCount;
    u32             RenderCommandsCap;
    
    gpu_command    *GpuCommands;
    u32             GpuCommandsCount;
    u32             GpuCommandsCap;
};

void InitFrameParams(frame_params *FrameParams);
void FreeFrameParams(frame_params *FrameParams);

void AddRenderCommand(frame_params *FrameParams, render_command Cmd);
void AddGpuCommand(frame_params *FrameParams, gpu_command Cmd);

#endif //ENGINE_FRAME_INFO_FRAME_PARAMS_H
