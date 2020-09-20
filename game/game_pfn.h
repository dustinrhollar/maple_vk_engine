/* date = August 19th 2020 9:03 pm */

#ifndef GAME_GAME_PFN_H
#define GAME_GAME_PFN_H


#ifdef __cplusplus
extern "C" 
{   // only need to export C interface if
    // used by C++ source code
#endif
    
#if defined(GAME_DLL_EXPORT)
    
#define GAME_API __declspec(dllexport)
#define GAME_CALL __cdecl
    
#else
    
#define GAME_API __declspec(dllimport)
#define GAME_CALL
    
#endif // GRAPHICS_DLL_EXPORT 
    
#define GAME_ENTRY(fn) GAME_API void fn(struct frame_params *FrameInfo)
    typedef void (GAME_CALL *PFN_game_entry)(struct frame_params *FrameInfo);
    
#ifdef __cplusplus
} // extern "C"
#endif

#endif //GAME_PFN_H
