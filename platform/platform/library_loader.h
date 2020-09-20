#ifndef PLATFORM_LIBRARY_LOADER_H
#define PLATFORM_LIBRARY_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif
    
    typedef struct graphics_api
    {
        
#define GRAPHICS_EXPORTED_FUNCTION(fn) PFN_##fn fn;
#include "../graphics/graphics_functions.inl"
        
    } graphics_api;
    
    typedef struct game_api
    {
        
#define GAME_EXPORTED_FUNCTION(fn) PFN_##fn fn;
#include "../game/game_pfn.inl"
        
    } game_api;
    
    extern graphics_api *Graphics;
    extern game_api     *Game;
    
#ifdef __cplusplus
}
#endif

#endif //PLATFORM_LIBRARY_LOADER_H
