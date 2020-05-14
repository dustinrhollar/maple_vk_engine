#ifndef PLATFORM_H
#define PLATFORM_H

/*

//-----------------------------------------------------------------------------------------------------------
// Input API



//-----------------------------------------------------------------------------------------------------------
// Platform API

// LOGGING/PRINTING
 
i32 PlatformFormatString(char *buff, i32 len, char* fmt, ...);

Formats a string with a provided format and a variable set of arguments.
The format string is written to buf, if there is enough space. The number
of characters required for the format is returned. It should be noted that
the return value is independent of the passed length and buffer size. This
is done so that if a user does not know the needed size, they can first call
the function to get the required space, allocate a buffer, and then call the
function a second time to format the string.

void PlatformPrintMessage(EConsoleColor text_color, EConsoleColor background_color, char* fmt, ...);

Print a message to the console/logger to STD_OUTPUT_HANDLE. A text color and
back ground color can be specified based on the enum in EConsoleColor. This
function calls PlatformFormatString under the hood.

void PlatformPrintError(EConsoleColor text_color, EConsoleColor background_color, char* fmt, ...);

Print a message to the console/logger to STD_ERROR_HANDLE. A text color and
back ground color can be specified based on the enum in EConsoleColor. This
function calls PlatformFormatString under the hood.

mformat

Macro function that is forwared to PlatformFormatString.

mprint(char *fmt, ...);

 Inline function that is forwarded to PlatformPrintMessage. Text color is set
to white and back ground color is set to Dark Grey.

mprinte(char *fmt, ...);

 Inline function that is forwarded to PlatformPrintError. Text color is set
to red and back ground color is set to Dark Grey.


// FILE/IO

NOTE(Dustin): File I/O related functions taken in directories/files
relatively to the executable unless otherwise specified.

- DynamicArray<jstring> PlatformFindAllFilesInDirectory(jstring &directory, jstring delimiter);

Retrieves all files in in a directory based a delimiteer. The delimiter can
be a file name or just an extension. Directories are ignored.

- jstring PlatformGetExeFilepath();

Gets the executable filepath. One Windows,  "\\" characters are changed to "/". The
filepath ends with "/".

- jstring PlatformLoadFile(jstring &filename);

Load a file into a jstring. If the file fails to load, then an error it output
and an empty string is returned.

- void PlatformWriteBufferToFile(jstring &file, void *buffer, u32 size);

Given a buffer, write it to a file. The file first checks if it it exists, and
if it does, the file if truncated. If the file is not found, then a new file is
created.

- void PlatformDeleteFile(jstring &file);

Delete the file passed as the parameter.

// TIMING

- u64 PlatformGetWallClock();

Get the current time as a uint64_t value..

- r32 PlatformGetSecondsElapsed(r32 start, u32 end);

Given a start and end time, return the seconds elapsed as a float.

// VULKAN

- const char* PlatformGetRequiredInstanceExtensions(bool validation_layers);

Get the platform specific KHR extension. On Windows this will be
VK_KHR_win32_surface and on Linux it will be VK_KHR_x11_surface
oor VK_KHR_xcb_surface.

- void PlatformGetClientWindowDimensions(u32 *width, u32 *height);

Get the width and height of the client window.

- void PlatformVulkanCreateSurface(VkSurfaceKHR *surface, VkInstance vulkan_instance);

Creates a VulkanSurfaceKHR based on the platform. On Windows, vkCreateWin32SurfaceKHR
is called and on Linux vkCreateX11SurfaceKHR or vkCreateXcbSurfaceKHR is called.

// ERROR HANDLING

- void PlatformRaiseError(ErrorCode error, char *fmt, ...);

NOT YET IMPLEMENTED.

// OTHER

- void PlatformExecuteCommand(jstring &system_cmd);

Executes a system command. This calling is blocking,
so the function will block until the command has
finished executing.

*/

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
    
    r32 TimeElapsed;
    
    bool RenderWireframe;
};

enum ErrorCode
{
    ERROR_CODE_NONE,
    ERROR_CODE_SEG,
};

enum class EConsoleColor
{
    White,
    DarkGrey,
    Grey,
    DarkRed,
    Red,
    DarkGreen,
    Green,
    DarkBlue,
    Blue,
    DarkCyan,
    Cyan,
    DarkPurple,
    Purple,
    DarkYellow,
    Yellow,
};

// Simpler versions of the platform print
// NOTE(Dustin): Formatting a string returns a
// jstring, which might be desired since a user cannot
// control the allocation of the formatted string.
// might want to change it so that it returns an int
// similar to snprintf (total that could be read if enough space)
// and pass a char* buffer as an argument
#define mformat PlatformFormatString
inline void mprint(char *fmt, ...);
inline void mprinte(char *fmt, ...);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)

// Custom I/O
#define PlatformFormatString                  Win32FormatString
#define PlatformPrintMessage                  Win32PrintMessage
#define PlatformPrintError                    Win32PrintError

#define PlatformGetExeFilepath                Win32GetExeFilepath
#define PlatformFindAllFilesInDirectory       Win32FindAllFilesInDirectory
#define PlatformWriteBufferToFile             Win32WriteBufferToFile
#define PlatformLoadFile                      Win32LoadFile
#define PlatformDeleteFile                    Win32DeleteFile
#define PlatformExecuteCommand                Win32ExecuteCommand
#define PlatformGetRequiredInstanceExtensions Win32GetRequiredInstanceExtensions
#define PlatformGetClientWindowDimensions     Win32GetClientWindowDimensions
#define PlatformVulkanCreateSurface           Win32VulkanCreateSurface

#define PlatformGetWallClock                  Win32GetWallClock
#define PlatformGetSecondsElapsed             Win32GetSecondsElapsed
#define PlatformRaiseError                    Win32RaiseError

i32 Win32FormatString(char *buff, i32 len, char* fmt, ...);
void Win32PrintMessage(EConsoleColor text_color, EConsoleColor background_color, char* fmt, ...);
void Win32PrintError(EConsoleColor text_color, EConsoleColor background_color, char* fmt, ...);

DynamicArray<jstring> Win32FindAllFilesInDirectory(jstring &directory, jstring delimiter);
jstring Win32GetExeFilepath();
jstring Win32LoadFile(jstring &filename);
void Win32WriteBufferToFile(jstring &file, void *buffer, u32 size);
void Win32DeleteFile(jstring &file);
void Win32ExecuteCommand(jstring &system_cmd);
u64 Win32GetWallClock();
r32 Win32GetSecondsElapsed(r32 start, u32 end);
void Win32RaiseError(ErrorCode error, char *fmt, ...);

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
#define PlatformGetRequiredInstanceExtensions XcbGetRequiredInstanceExtensions
#define PlatformGetClientWindowDimensions     XcbGetClientWindowDimensions
#define PlatformVulkanCreateSurface           XcbVulkanCreateSurface

jstring XcbGetExeFilepath();
DynamicArray<jstring> XcbFindAllFilesInDirectory(jstring &directory, jstring delimiter);
void XcbWriteBufferToFile(jstring &file, void *buffer, u32 size);
jstring XcbLoadFile(jstring &directory, jstring &filename);
void XcbDeleteFile(jstring &file);
void XcbExecuteCommand(jstring &system_cmd);

const char *XcbGetRequiredInstanceExtensions(bool validation_layers);
void XcbGetClientWindowDimensions(u32 *width, u32 *height);
void XcbVulkanCreateSurface(VkSurfaceKHR *surface, VkInstance vulkan_instance);

#endif

#endif //PLATFORM_H
