#ifndef MAPLE_PLATFORM_PLATFORM_H
#define MAPLE_PLATFORM_PLATFORM_H

// opaque wrapper around the Window handle. 
// can be coerced to the handle if the platform is known.
// for example, platform_window on win32 is:
//    struct { HWND Window; } platform_window;
typedef struct platform_window* window_t;

struct window_rect
{
    u64 Left;
    u64 Top;
    u64 Right;
    u64 Bottom;
};

// opaque wrapper around a file handle. the file struct
// also contains two types of internal buffers in order
// to store memory.
// 1. Tagged Heap Block: this is a linear allocator of size
//    2 MB. All files less than this size will use this memory
//    for reading. When writing to a file, this block is ALWAYS
//    used. If a write to this buffer fills the available memory,
//    then the buffer is automatically flushed to a file. However,
//    it is up to the user to flush the buffer when they are done
//    writing to the file.
// 2. Allocated Block: This memory is used for large file sizes
//    up to 2^32 - 1. This upper bound is dictated by the Win32
//    API. The memory for this block is obtained through VirtualAlloc.
//    The Allocated block is only for file reading and is not used
//    for writing to files. For file writes over 2MB, it is currently
//    advised that a user batch 2MB file writes.
// Currently, only 10 files are allowed to be open at once. This is
// an arbritrary limit and can be increased in the future if necessary.
typedef struct file* file_t;

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

//~ File I/O

// OPENING/CLOSING A FILE

file_t PlatformLoadFile(const char *Filename, bool Append = false);
file_t PlatformOpenFile(const char *Filename, bool Append = false);
// Flush the memory buffer to the file. 
void PlatformFlushFile(file_t File);
void PlatformCloseFile(file_t File);

// MOVING ABOUT A FILE

// "Save" the current offset into a file. This is particularly useful
// if a user is scanning over and plan to rewind their position. Currently,
// only 5 savepoints are allowed. The Savepoints structure acts as a stack. 
bool PlatformFileSetSavepoint(file_t File);
// Restore the save point at the top of the stack.
bool PlatformFileRestoreSavepoint(file_t File);
// returns the number of bytes read from the file - irregardless if the buffer size was big enough
// same behavior as the snprintf family of functions.
// TODO(Dustin): Have a function like this ^ ?
// Read

// FILE READ OPERATIONS

void PlatformReadFile(file_t File, void *Result, u32 Len, const char *Fmt);

// WRITING TO A FILE

// Retrieve a pointer to the underlying buffer being used.
void PlatformWriteToFile(file_t File, const char *Fmt, ...);
// Write a buffer to a file while not caring about its contents. 
void PlatformWriteBinaryStreamToFile(file_t File, void *DataPtr, u64 DataSize);
// Allows for arrays (or singular value) of a particular type to be written 
// to a file. 
void PlatformWriteBinaryToFile(file_t File, const char *Fmt, void *Data, u32 DataLen);

// RETRIEVE FILE INFO

void* GetFileBuffer(file_t File);
// Get the current size of the file. Only useful when reading files.
u64 PlatformGetFileSize(file_t File);

// EDIT FILE LOCATION/DATA

// Copy a file to the destination if:
// 1. The file does not currently exist at the destination
// 2. If the file has been changed since the last copy.
void PlatformCopyFileIfChanged(const char *Dst, const char *Src);

//~ Timing
u64 PlatformGetWallClock();
r32 PlatformGetElapsedSeconds(u64 Start, u64 End);

//~ Bit shifting
__inline u32 PlatformClzl(u64 Value);
u32 PlatformCtzl(u64 Value);

//~ Retrieve the width and height of the the client window. 
void PlatformGetClientWindowDimensions(u32 *Width, u32 *Height);
// TODO(Dustin): Get working...
void PlatformGetClientWindow(platform_window *Window);
window_rect PlatformGetClientWindowRect();

//~ Game API. Any client wanting to use the engine with the intended use case
// must have these functions exported in their DLL.

struct frame_params;
#define GAME_STAGE_ENTRY(fn) void fn(frame_params *FrameParams)
typedef GAME_STAGE_ENTRY(game_stage_entry);

#endif //MAPLE_PLATFORM_PLATFORM_H
