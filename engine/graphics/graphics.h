#ifndef ENGINE_GRAPGICS_GRAPHICS_H
#define ENGINE_GRAPGICS_GRAPHICS_H

// NOTE(Dustin): Not IN USE
void GraphicsInit(window_t PlatformWindow,
                  u32      Width,
                  u32      Height,
                  u32      RefreshRate);
void GraphicsFree();
void GraphicsResize(u32 Width, u32 Height);


#endif //ENGINE_GRAPGICS_GRAPHICS_H
