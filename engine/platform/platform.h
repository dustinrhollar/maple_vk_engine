#ifndef MAPLE_PLATFORM_PLATFORM_H
#define MAPLE_PLATFORM_PLATFORM_H

typedef struct platform_window* window_t;
typedef struct file* file_t;

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

// Platform Memory allocation
void* PlatformRequestMemory(u64 Size);
void PlatformReleaseMemory(void *Ptr, u64 Size);

// Log/Printing
#define mformat PlatformFormatString
inline void mprint(char *fmt, ...);
inline void mprinte(char *fmt, ...);
i32  PlatformFormatString(char *buff, i32 len, char* fmt, ...);
void PlatformPrintMessage(console_color text_color, console_color background_color, char* fmt, ...);
void PlatformPrintError(console_color text_color, console_color background_color, char* fmt, ...);

// File I/O
file_t PlatformLoadFile(const char *Filename, bool Append = false);
file_t PlatformOpenFile(const char *Filename, bool Append = false);
void* GetFileBuffer(file_t File);
u64 PlatformGetFileSize(file_t File);
void PlatformFlushFile(file_t File);
void PlatformCloseFile(file_t File);
void CopyFileIfChanged(const char *Dst, const char *Src);

// Timing
u64 PlatformGetWallClock();
r32 PlatformGetElapsedSeconds(u64 Start, u64 End);

// Bit shifting
__inline u32 PlatformClzl(u64 Value);
u32 PlatformCtzl(u64 Value);

void PlatformGetClientWindowDimensions(u32 *Width, u32 *Height);

#endif //MAPLE_PLATFORM_PLATFORM_H
