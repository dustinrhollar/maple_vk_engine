#ifndef GRAPHICS_PLATFORM_H
#define GRAPHICS_PLATFORM_H

// opaque wrapper around the Window handle. 
// can be coerced to the handle if the platform is known.
// for example, platform_window on win32 is:
//    struct { HWND Window; } platform_window;
typedef struct platform_window* window_t;
typedef struct file* file_t;

typedef struct 
{
    u64 Left;
    u64 Top;
    u64 Right;
    u64 Bottom;
} window_rect;

// A set of enumeration value to dictate the text/background color
// for console logging. 
typedef enum
{
    ConsoleColor_White,
    ConsoleColor_DarkGrey,
    ConsoleColor_Grey,
    ConsoleColor_DarkRed,
    ConsoleColor_Red,
    ConsoleColor_DarkGreen,
    ConsoleColor_Green,
    ConsoleColor_DarkBlue,
    ConsoleColor_Blue,
    ConsoleColor_DarkCyan,
    ConsoleColor_Cyan,
    ConsoleColor_DarkPurple,
    ConsoleColor_Purple,
    ConsoleColor_DarkYellow,
    ConsoleColor_Yellow,
} console_color;

//~ Allocated/Free memory. This is done through platform specific functions
// (i.e. VirtualAlloc or mmap)
void* PlatformRequestMemory(u64 Size);
void PlatformReleaseMemory(void *Ptr, u64 Size);

//~ Log/Printing
#define mformat PlatformFormatString
inline void mprint(char *fmt, ...);
inline void mprinte(char *fmt, ...);
// Formats a string with the given format. 
// same behavior as the snprintf family of functions.
i32  PlatformFormatString(char *buff, i32 len, char* fmt, ...);
// Print a formated message/error with the given text/background colors.
void PlatformPrintMessage(console_color text_color, console_color background_color, char* fmt, ...);
void PlatformPrintError(console_color text_color, console_color background_color, char* fmt, ...);

// opens an error window with the formatted message and then exits the application
void PlatformFatalError(char *Fmt, ...);

//~ Vulkan Setup functions

void PlatformGetClientWindowDimensions(u32 *Width, u32 *Height);
window_rect PlatformGetClientWindowRect();

const char* PlatformGetRequiredInstanceExtensions(bool validation_layers);
void PlatformVulkanCreateSurface(VkSurfaceKHR *surface, VkInstance vulkan_instance);

void PlatformSetClientWindow(platform_window *Window);

//~ File I/O

file_t PlatformLoadFile(char *Filename, bool Append = false);
file_t PlatformOpenFile(char *Filename, bool Append = false);
void PlatformCloseFile(file_t File);

void* GetFileBuffer(file_t File);
// Get the current size of the file. Only useful when reading files.
u64 PlatformGetFileSize(file_t File);

//~ Fast Bit operations

// For 32 bit values
u32 PlatformClz(u32 Value);
u32 PlatformCtz(u32 Value);

// For 64 bit values
u32 PlatformClzl(u64 Value);
u32 PlatformCtzl(u64 Value);

#endif //GRAPHICS_PLATFORM_H
