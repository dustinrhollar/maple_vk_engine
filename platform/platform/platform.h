#ifndef MAPLE_PLATFORM_PLATFORM_H
#define MAPLE_PLATFORM_PLATFORM_H

typedef enum input_key
{
    // Keyboard
    Key_w           = BIT(0),
    Key_s           = BIT(1),
    Key_a           = BIT(2),
    Key_d           = BIT(3),
    
    // Arrow Keys
    Key_Up          = BIT(4),
    Key_Down        = BIT(5),
    Key_Left        = BIT(6),
    Key_Right       = BIT(7),
    
    // Special keys
    Key_Alt         = BIT(8),
    Key_Shift       = BIT(9),
    
    // Mouse
    Key_LeftButton  = BIT(10),
    Key_RightButton = BIT(11),
    
    // F Keys
    Key_F1          = BIT(12), 
    Key_F2          = BIT(13),
    Key_F3          = BIT(14),
    Key_F4          = BIT(15),
    Key_F5          = BIT(16),
    
    // Number Keys
    Key_0           = BIT(17),
    
} input_key;

typedef struct mouse_movement
{
    r32 XPos;
    r32 YPos;
} mouse_movement;

typedef enum input_type
{
    Input_KeyBoard,
    Input_Mouse,
} input_type;

typedef struct input
{
    input_key      KeyPress;
    mouse_movement Movement;
} input;

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
void mprint(char *fmt, ...);
void mprinte(char *fmt, ...);
// Formats a string with the given format. 
// same behavior as the snprintf family of functions.
i32  PlatformFormatString(char *buff, i32 len, char* fmt, ...);
// Print a formated message/error with the given text/background colors.
void PlatformPrintMessage(console_color text_color, console_color background_color, char* fmt, ...);
void PlatformPrintError(console_color text_color, console_color background_color, char* fmt, ...);

// opens an error window with the formatted message and then exits the application
void PlatformFatalError(char *Fmt, ...);


//~ File I/O

file_t PlatformLoadFile(char *Filename, bool Append);
file_t PlatformOpenFile(char *Filename, bool Append);
void PlatformCloseFile(file_t File);
void PlatformFlushFile(file_t File);
void PlatformWriteFile(file_t File, char *Fmt, ...);

void* GetFileBuffer(file_t File);
// Get the current size of the file. Only useful when reading files.
u64 PlatformGetFileSize(file_t File);


#if 0

// OPENING/CLOSING A FILE

// Flush the memory buffer to the file. 
void PlatformFlushFile(file_t File);

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

// EDIT FILE LOCATION/DATA

// Copy a file to the destination if:
// 1. The file does not currently exist at the destination
// 2. If the file has been changed since the last copy.
void PlatformCopyFileIfChanged(const char *Dst, const char *Src);

#endif

//~ Timing
u64 PlatformGetWallClock();
r32 PlatformGetElapsedSeconds(u64 Start, u64 End);

//~ Bit shifting

u32 PlatformClz(u32 Value);
u32 PlatformCtz(u32 Value);
__inline u32 PlatformClzl(u64 Value);
u32 PlatformCtzl(u64 Value);

//~ Retrieve the width and height of the the client window. 
void PlatformGetClientWindowDimensions(u32 *Width, u32 *Height);
// TODO(Dustin): Get working...
//void PlatformGetClientWindow(platform_window *Window);
window_rect PlatformGetClientWindowRect();


//~ Function pointers for the platform api

typedef file_t (*platform_open_file)(char *Filename, bool Append);
typedef file_t (*platform_load_file)(char *Filename, bool Append);
typedef void (*platform_close_file)(file_t File);
typedef void (*platform_flush_file)(file_t File);
typedef void (*platform_write_file)(file_t File, char *Fmt, ...);
typedef void* (*platform_get_file_buffer)(file_t File);
typedef u64 (*platform_get_file_size)(file_t File);

typedef void (*platform_mprint)(char *Fmt, ...);

typedef struct platform
{
    struct memory *Memory;
    
    // Logging
    platform_mprint          mprint;
    
    // File Api
    platform_open_file       open_file;
    platform_load_file       load_file;
    platform_close_file      close_file;
    platform_flush_file      flush_file;
    platform_write_file      write_file;
    platform_get_file_buffer get_file_buffer;
    platform_get_file_size   get_file_size;
    
} platform;

extern platform *PlatformApi;

#endif //MAPLE_PLATFORM_PLATFORM_H
