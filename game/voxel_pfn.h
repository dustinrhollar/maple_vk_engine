/* date = August 19th 2020 9:03 pm */

#ifndef GAME_VOXEL_PFN_H
#define GAME_VOXEL_PFN_H


#ifdef __cplusplus
extern "C" 
{   // only need to export C interface if
    // used by C++ source code
#endif
    
#if defined(VOXEL_DLL_EXPORT)
    
#define VOXEL_API __declspec(dllexport)
#define VOXEL_CALL __cdecl
    
#else
    
#define VOXEL_API __declspec(dllimport)
#define VOXEL_CALL
    
#endif // GRAPHICS_DLL_EXPORT 
    
#define VOXEL_ENTRY(fn) VOXEL_API void fn(struct frame_params *FrameInfo)
    typedef void (VOXEL_CALL *PFN_voxel_entry)(struct frame_params *FrameInfo);
    
#define VOXEL_SHUTDOWN(fn) VOXEL_API void fn()
    typedef void (VOXEL_CALL *PFN_voxel_shutdown)();
    
#ifdef __cplusplus
} // extern "C"
#endif

#endif //VOXEL_PFN_H
