#ifndef ENGINE_GRAPHICS_RENDER_COMMAND_H
#define ENGINE_GRAPHICS_RENDER_COMMAND_H

enum render_command_type
{
    RenderCmd_Draw,
};

struct render_command_draw
{
    asset_id Asset;
};

struct render_command
{
    render_command_type Type;
    
    union
    {
        render_command_draw DrawCmd;
    };
};

#endif //ENGINE_GRAPHICS_RENDER_COMMAND_H
