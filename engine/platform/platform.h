#ifndef PLATFORM_H
#define PLATFORM_H

enum KeyInput
{
    KEY_PRESS_W = 1<<0,
    KEY_PRESS_S = 1<<1,
    KEY_PRESS_A = 1<<2,
    KEY_PRESS_D = 1<<3,
    KEY_PRESS_1 = 1<<4,
    KEY_PRESS_2 = 1<<5,
    KEY_PRESS_3 = 1<<6,
    KEY_PRESS_4 = 1<<7,
    KEY_PRESS_UP    = 1<<8,
    KEY_PRESS_DOWN  = 1<<9,
    KEY_PRESS_LEFT  = 1<<10,
    KEY_PRESS_RIGHT = 1<<11,
    
    KEY_PRESS_I = 1<<12,
    KEY_PRESS_O = 1<<13,
    KEY_PRESS_P = 1<<14,
};

//jstring PlatformGetExeFilepath();
//DynamicArray<jstring> PlatformFindAllFilesInDirectory(jstring &directory, jstring delimiter);
//void PlatformWriteBufferToFile(jstring &file, void *buffer, u32 size);
//jstring PlatformLoadFile(jstring &directory, jstring &filename);
//void PlatformDeleteFile(jstring &file);
//void PlatformExecuteCommand(jstring &system_cmd);
//void PlatformEnableLogging();

//const char *PlatformGetRequiredInstanceExtensions(bool validation_layers);
//void PlatformGetClientWindowDimensions(u32 *width, u32 *height);
//void PlatformVulkanCreateSurface(VkSurfaceKHR *surface, VkInstance vulkan_instance);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)


#define PlatformGetExeFilepath                Win32GetExeFilepath
#define PlatformFindAllFilesInDirectory       Win32FindAllFilesInDirectory
#define PlatformWriteBufferToFile             Win32WriteBufferToFile
#define PlatformLoadFile                      Win32LoadFile
#define PlatformDeleteFile                    Win32DeleteFile
#define PlatformExecuteCommand                Win32ExecuteCommand
#define PlatformEnableLogging                 Win32EnableLogging
#define PlatformGetRequiredInstanceExtensions Win32GetRequiredInstanceExtensions
#define PlatformGetClientWindowDimensions     Win32GetClientWindowDimensionsb
#define PlatformVulkanCreateSurface           Win32VulkanCreateSurface


DynamicArray<jstring> Win32FindAllFilesInDirectory(jstring &directory, jstring delimiter);
jstring Win32GetExeFilepath();
jstring Win32LoadFile(jstring &directory, jstring &filename);
void Win32WriteBufferToFile(jstring &file, void *buffer, u32 size);
void Win32DeleteFile(jstring &file);
void Win32ExecuteCommand(jstring &system_cmd);
void Win32EnableLogging();

const char* Win32GetRequiredInstanceExtensions(bool validation_layers);
void Win32GetClientWindowDimensionsb(u32 *width, u32 *height);
void Win32VulkanCreateSurface(VkSurfaceKHR *surface, VkInstance vulkan_instance);

#else

#define PlatformGetExeFilepath                X11GetExeFilepath
#define PlatformFindAllFilesInDirectory       X11FindAllFilesInDirectory
#define PlatformWriteBufferToFile             X11WriteBufferToFile
#define PlatformLoadFile                      X11LoadFile
#define PlatformDeleteFile                    X11DeleteFile
#define PlatformExecuteCommand                X11ExecuteCommand
#define PlatformEnableLogging                 X11EnableLogging
#define PlatformGetRequiredInstanceExtensions X11GetRequiredInstanceExtensions
#define PlatformGetClientWindowDimensions     X11GetClientWindowDimensionsb
#define PlatformVulkanCreateSurface           X11VulkanCreateSurface

jstring X11GetExeFilepath();
DynamicArray<jstring> X11FindAllFilesInDirectory(jstring &directory, jstring delimiter);
void X11WriteBufferToFile(jstring &file, void *buffer, u32 size);
jstring X11LoadFile(jstring &directory, jstring &filename);
void X11DeleteFile(jstring &file);
void X11ExecuteCommand(jstring &system_cmd);
void X11EnableLogging();

const char *X11GetRequiredInstanceExtensions(bool validation_layers);
void X11GetClientWindowDimensionsb(u32 *width, u32 *height);
void X11VulkanCreateSurface(VkSurfaceKHR *surface, VkInstance vulkan_instance);

#endif



#endif //PLATFORM_H
