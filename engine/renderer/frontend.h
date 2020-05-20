#ifndef ENGINE_RENDERER_FRONTEND_H
#define ENGINE_RENDERER_FRONTEND_H

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

void RenderStageEntry();

#endif //ENGINE_RENDERER_FRONTEND_H
