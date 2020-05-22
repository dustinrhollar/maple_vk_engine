#ifndef ENGINE_RENDERER_FRONTEND_H
#define ENGINE_RENDERER_FRONTEND_H

struct frame_params;

enum render_command_type
{
    RenderCmd_Draw,
    RenderCmd_LoadAsset,
};

struct render_command
{
    render_command_type Type;
    void *Data;
};

void RenderStageInit(frame_params *FrameParams);
void RenderStageEntry(frame_params *FrameParams);

#endif //ENGINE_RENDERER_FRONTEND_H
