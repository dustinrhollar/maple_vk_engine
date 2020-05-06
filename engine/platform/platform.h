#ifndef PLATFORM_H
#define PLATFORM_H

struct KeyboardInput
{
    u64 KEY_A:1;
    u64 KEY_B:1;
    u64 KEY_C:1;
    u64 KEY_D:1;
    u64 KEY_E:1;
    u64 KEY_F:1;
    u64 KEY_G:1;
    u64 KEY_H:1;
    u64 KEY_I:1;
    u64 KEY_J:1;
    u64 KEY_K:1;
    u64 KEY_L:1;
    u64 KEY_M:1;
    u64 KEY_N:1;
    u64 KEY_O:1;
    u64 KEY_P:1;
    u64 KEY_Q:1;
    u64 KEY_R:1;
    u64 KEY_S:1;
    u64 KEY_T:1;
    u64 KEY_U:1;
    u64 KEY_V:1;
    u64 KEY_W:1;
    u64 KEY_X:1;
    u64 KEY_Y:1;
    u64 KEY_Z:1;
    
    u64 LSHIFT:1;
    u64 LCONTROL:1;
    u64 LALT:1;
    
    u64 KEY_F1:1;
    u64 KEY_F2:1;
    u64 KEY_F3:1;
    u64 KEY_F4:1;
    u64 KEY_F5:1;
    u64 KEY_F6:1;
    u64 KEY_F7:1;
    u64 KEY_F8:1;
    u64 KEY_F9:1;
    u64 KEY_F10:1;
    u64 KEY_F11:1;
    u64 KEY_F12:1;
    
    u64 KEY_ARROW_UP:1;
    u64 KEY_ARROW_DOWN:1;
    u64 KEY_ARROW_LEFT:1;
    u64 KEY_ARROW_RIGHT:1;
    
    u64 Padding:19;
};

struct MouseInput
{
    u32 MouseXPos;
    u32 MouseYPos;
    
    u8 MOUSE_LEFT:1;
    u8 MOUSE_RIGHT:1;
    u8 Padding:6;
};

struct FrameInput
{
    KeyboardInput Keyboard;
    MouseInput    Mouse;
};

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)

#define PlatformGetExeFilepath                Win32GetExeFilepath
#define PlatformFindAllFilesInDirectory       Win32FindAllFilesInDirectory
#define PlatformWriteBufferToFile             Win32WriteBufferToFile
#define PlatformLoadFile                      Win32LoadFile
#define PlatformDeleteFile                    Win32DeleteFile
#define PlatformExecuteCommand                Win32ExecuteCommand
#define PlatformEnableLogging                 Win32EnableLogging
#define PlatformGetRequiredInstanceExtensions Win32GetRequiredInstanceExtensions
#define PlatformGetClientWindowDimensions     Win32GetClientWindowDimensions
#define PlatformVulkanCreateSurface           Win32VulkanCreateSurface

DynamicArray<jstring> Win32FindAllFilesInDirectory(jstring &directory, jstring delimiter);
jstring Win32GetExeFilepath();
jstring Win32LoadFile(jstring &directory, jstring &filename);
void Win32WriteBufferToFile(jstring &file, void *buffer, u32 size);
void Win32DeleteFile(jstring &file);
void Win32ExecuteCommand(jstring &system_cmd);
void Win32EnableLogging();

const char* Win32GetRequiredInstanceExtensions(bool validation_layers);
void Win32GetClientWindowDimensions(u32 *width, u32 *height);
void Win32VulkanCreateSurface(VkSurfaceKHR *surface, VkInstance vulkan_instance);

#else

#define PlatformGetExeFilepath                XcbGetExeFilepath
#define PlatformFindAllFilesInDirectory       XcbFindAllFilesInDirectory
#define PlatformWriteBufferToFile             XcbWriteBufferToFile
#define PlatformLoadFile                      XcbLoadFile
#define PlatformDeleteFile                    XcbDeleteFile
#define PlatformExecuteCommand                XcbExecuteCommand
#define PlatformEnableLogging                 XcbEnableLogging
#define PlatformGetRequiredInstanceExtensions XcbGetRequiredInstanceExtensions
#define PlatformGetClientWindowDimensions     XcbGetClientWindowDimensions
#define PlatformVulkanCreateSurface           XcbVulkanCreateSurface

jstring XcbGetExeFilepath();
DynamicArray<jstring> XcbFindAllFilesInDirectory(jstring &directory, jstring delimiter);
void XcbWriteBufferToFile(jstring &file, void *buffer, u32 size);
jstring XcbLoadFile(jstring &directory, jstring &filename);
void XcbDeleteFile(jstring &file);
void XcbExecuteCommand(jstring &system_cmd);
void XcbEnableLogging();

const char *XcbGetRequiredInstanceExtensions(bool validation_layers);
void XcbGetClientWindowDimensions(u32 *width, u32 *height);
void XcbVulkanCreateSurface(VkSurfaceKHR *surface, VkInstance vulkan_instance);

#endif



#endif //PLATFORM_H
