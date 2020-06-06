#include "spriv/spirv_glsl.hpp"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <Tchar.h>
#include <strsafe.h>
#include <stdio.h>

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 512
#endif


//~ Required C++ Headers
//~ CLib  Headers
#include <assert.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Used for determining if a registered component
// inherits from IComponent
// needed for placement new
#include <new>
// NOTE(Dustin): Maybe can replace with hashtable
#include <set>
#include <new>
// NOTE(Dustin): Handle this better: Used for VK_CHECK_RESULT
#include <stdexcept>
// NOTE(Dustin): Is this still necessary?
#include <optional>

#include "../../engine/utils/maple_types.h"

//~ Memory Manager Headers
#include "../../engine/mm/allocator.h"
#include "../../engine/mm/linear_allocator.h"
#include "../../engine/mm/free_list_allocator.h"
#include "../../engine/mm/mm.h"

#define MAPLE_HASHTABLE_IMPLEMENTATION
#define MAPLE_JSTRING_IMPLEMENTATION
#define MAPLE_HASH_FUNCTION_IMPLEMENTATION
#define MAPLE_DYNAMIC_ARRAY_IMPLEMENTATION
#define MAPLE_VECTOR_MATH_IMPLEMENTATION
#include "../../engine/utils/dynamic_array.h"
#include "../../engine/utils/hash_functions.h"
#include "../../engine/utils/jstring.h"
#include "../../engine/utils/hashtable.h"
#include "../../engine/utils/vector_math.h"

//~ Platform stuff


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

HANDLE Win32OpenFile(const char *Filename, bool IsWrite);
void Win32CloseFile(HANDLE FileHandle);
void Win32WriteToFile(HANDLE FileHandle, char *Fmt, ...);

#define PlatformLoadFile Win32LoadFile
#define mformat Win32FormatString
inline void mprint(char *fmt, ...);
inline void mprinte(char *fmt, ...);


//~ File IO Operations (OS Independent)
#include "../../engine/io/file.h"
#include "../../engine/io/file.cpp"

//~ MM Source Files
#include "../../engine/mm/linear_allocator.cpp"
#include "../../engine/mm/free_list_allocator.cpp"
#include "../../engine/mm/mm.cpp"

//~  Config Parser
#include "../../engine/config/config_parser.h"
#include "../../engine/config/config_parser.cpp"

//~ Generator Src files
#include "refl_gen.h"
#include "refl_gen.cpp"

// source: Windows API doc: https://docs.microsoft.com/en-us/windows/win32/fileio/opening-a-file-for-reading-or-writing
file_internal void DisplayError(LPTSTR lpszFunction)
// Routine Description:
// Retrieve and output the system error message for the last-error code
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();
    
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  dw,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf,
                  0,
                  NULL );
    
    lpDisplayBuf =
        (LPVOID)LocalAlloc( LMEM_ZEROINIT,
                           ( lstrlen((LPCTSTR)lpMsgBuf)
                            + lstrlen((LPCTSTR)lpszFunction)
                            + 40) // account for format string
                           * sizeof(TCHAR) );
    
    if (FAILED( StringCchPrintf((LPTSTR)lpDisplayBuf,
                                LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                                TEXT("%s failed with error code %d as follows:\n%s"),
                                lpszFunction,
                                dw,
                                lpMsgBuf)))
    {
        mprinte("FATAL ERROR: Unable to output error code.\n");
    }
    
    _tprintf(TEXT("ERROR: %s\n"), (LPCTSTR)lpDisplayBuf);
    
    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

//~ Logging

struct Win32StandardStream
{
    HANDLE handle; // Stream handle (STD_OUTPUT_HANDLE or STD_ERROR_HANDLE).
    bool is_redirected; // True if redirected to file.
    bool is_wide; // True if appending to a UTF-16 file.
    bool is_little_endian; // True if file is UTF-16 little endian.
};

file_internal bool RedirectConsoleIO()
{
    bool result = true;
    FILE* fp;
    
    // Redirect STDIN if the console has an input handle
    if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
        if (freopen_s(&fp, "CONIN$", "r", stdin) != 0)
        result = false;
    else
        setvbuf(stdin, NULL, _IONBF, 0);
    
    // Redirect STDOUT if the console has an output handle
    if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
        if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0)
        result = false;
    else
        setvbuf(stdout, NULL, _IONBF, 0);
    
    // Redirect STDERR if the console has an error handle
    if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
        if (freopen_s(&fp, "CONOUT$", "w", stderr) != 0)
        result = false;
    else
        setvbuf(stderr, NULL, _IONBF, 0);
    
    return result;
}

// Sets up a standard stream (stdout or stderr).
file_internal Win32StandardStream Win32GetStandardStream(u32 stream_type)
{
    Win32StandardStream result = {};
    
    // If we don't have our own stream and can't find a parent console,
    // allocate a new console.
    result.handle = GetStdHandle(stream_type);
    if (!result.handle || result.handle == INVALID_HANDLE_VALUE)
    {
        if (!AttachConsole(ATTACH_PARENT_PROCESS))
        {
            AllocConsole();
            RedirectConsoleIO();
        }
        result.handle = GetStdHandle(stream_type);
    }
    
    // Check if the stream is redirected to a file. If it is, check if
    // the file already exists. If so, parse the encoding.
    if (result.handle != INVALID_HANDLE_VALUE)
    {
        DWORD type = GetFileType(result.handle) & (~FILE_TYPE_REMOTE);
        DWORD dummy;
        result.is_redirected = (type == FILE_TYPE_CHAR) ? !GetConsoleMode(result.handle, &dummy) : true;
        if (type == FILE_TYPE_DISK)
        {
            LARGE_INTEGER file_size;
            GetFileSizeEx(result.handle, &file_size);
            if (file_size.QuadPart > 1)
            {
                u16 bom = 0;
                SetFilePointerEx(result.handle, {}, 0, FILE_BEGIN);
                ReadFile(result.handle, &bom, 2, &dummy, 0);
                SetFilePointerEx(result.handle, {}, 0, FILE_END);
                result.is_wide = (bom == (u16)0xfeff || bom == (u16)0xfffe);
                result.is_little_endian = (bom == (u16)0xfffe);
            }
        }
    }
    return result;
}

// Translates foreground/background color into a WORD text attribute.
file_internal WORD Win32TranslateConsoleColors(EConsoleColor text_color, EConsoleColor background_color)
{
    WORD result = 0;
    switch (text_color)
    {
        case EConsoleColor::White:
        result |=  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::DarkGrey:
        result |= FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::Grey:
        result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
        case EConsoleColor::DarkRed:
        result |= FOREGROUND_RED;
        break;
        case EConsoleColor::Red:
        result |= FOREGROUND_RED | FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::DarkGreen:
        result |= FOREGROUND_GREEN;
        break;
        case EConsoleColor::Green:
        result |= FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::DarkBlue:
        result |= FOREGROUND_BLUE;
        break;
        case EConsoleColor::Blue:
        result |= FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::DarkCyan:
        result |= FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
        case EConsoleColor::Cyan:
        result |= FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::DarkPurple:
        result |= FOREGROUND_RED | FOREGROUND_BLUE;
        break;
        case EConsoleColor::Purple:
        result |= FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::DarkYellow:
        result |= FOREGROUND_RED | FOREGROUND_GREEN;
        break;
        case EConsoleColor::Yellow:
        result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
        default:
        break;
    }
    
    switch (background_color)
    {
        case EConsoleColor::White:
        result |=  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::DarkGrey:
        result |=  FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::Grey:
        result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
        case EConsoleColor::DarkRed:
        result |= FOREGROUND_RED;
        break;
        case EConsoleColor::Red:
        result |= FOREGROUND_RED | FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::DarkGreen:
        result |= FOREGROUND_GREEN;
        break;
        case EConsoleColor::Green:
        result |= FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::DarkBlue:
        result |= FOREGROUND_BLUE;
        break;
        case EConsoleColor::Blue:
        result |= FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::DarkCyan:
        result |= FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
        case EConsoleColor::Cyan:
        result |= FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::DarkPurple:
        result |= FOREGROUND_RED | FOREGROUND_BLUE;
        break;
        case EConsoleColor::Purple:
        result |= FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case EConsoleColor::DarkYellow:
        result |= FOREGROUND_RED | FOREGROUND_GREEN;
        break;
        case EConsoleColor::Yellow:
        result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
        default:
        break;
    }
    
    return result;
}

// Prints a message to a platform stream. If the stream is a console, uses
// supplied colors.
file_internal void Win32PrintToStream(const char* message, Win32StandardStream stream, EConsoleColor text_color, EConsoleColor background_color)
{
    
    // If redirected, write to a file instead of console.
    DWORD dummy;
    if (stream.is_redirected)
    {
        if (stream.is_wide)
        {
            static wchar_t buf[LOG_BUFFER_SIZE];
            i32 required_size = MultiByteToWideChar(CP_UTF8, 0, message, -1, 0, 0) - 1;
            i32 offset;
            for (offset = 0; offset + LOG_BUFFER_SIZE , required_size; offset += LOG_BUFFER_SIZE)
            {
                // TODO(Matt): Little endian BOM.
                MultiByteToWideChar(CP_UTF8, 0, &message[offset], LOG_BUFFER_SIZE, buf, LOG_BUFFER_SIZE);
                WriteFile(stream.handle, buf, LOG_BUFFER_SIZE * 2, &dummy, 0);
            }
            i32 mod = required_size % LOG_BUFFER_SIZE;
            i32 size = MultiByteToWideChar(CP_UTF8, 0, &message[offset], mod, buf, LOG_BUFFER_SIZE) * 2;
            WriteFile(stream.handle, buf, size, &dummy, 0);
        }
        else
        {
            WriteFile(stream.handle, message, (DWORD)strlen(message), &dummy, 0);
        }
    }
    else
    {
        WORD attribute = Win32TranslateConsoleColors(text_color, background_color);
        SetConsoleTextAttribute(stream.handle, attribute);
        WriteConsole(stream.handle, message, (DWORD)strlen(message), &dummy, 0);
        attribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        SetConsoleTextAttribute(stream.handle, attribute);
    }
}

i32 __Win32FormatString(char *buff, i32 len, char *fmt, va_list list)
{
    // if a caller doesn't actually know the length of the
    // format list, and is querying for the required size,
    // attempt to format the string into the buffer first
    // before copying in the chars.
    //
    // This handles the case where the buffer is declared like:
    //     char *buff = nullptr;
    char test_buff[12];
    va_list cpy;
    va_copy(cpy, list);
    u32 needed_chars = vsnprintf(NULL, 0, fmt, cpy);
    va_end(cpy);
    
    if (needed_chars < len)
    {
        needed_chars = vsnprintf(buff, len, fmt, list);
    }
    
    return needed_chars;
    
#if 0
    // since jstring contains a stack allocated buffer, we might not
    // actually need to resize
    if (needed_chars >= sizeof(result.sptr))
    {
        result = InitJString(needed_chars);
        
        needed_chars = vsnprintf(result.hptr, needed_chars, fmt, list);
        result.len = needed_chars;
        
        if ((result.heap) && result.len > result.reserved_heap_size)
        {
            // TODO(Dustin): Use new print methods
            mprinte("Failed to format a string!\n");
            result.Clear();
        }
    }
#endif
}

void __Win32PrintMessage(EConsoleColor text_color, EConsoleColor background_color, char *fmt, va_list args)
{
    char *message = nullptr;
    int chars_read = 1 + __Win32FormatString(message, 1, fmt, args);
    message = talloc<char>(chars_read);
    __Win32FormatString(message, chars_read, fmt, args);
    
    // If we are in the debugger, output there.
    if (IsDebuggerPresent())
    {
        OutputDebugStringA(message);
    }
    else
    {
        // Otherwise, output to stdout.
        local_persist Win32StandardStream stream = Win32GetStandardStream(STD_OUTPUT_HANDLE);
        Win32PrintToStream(message, stream, text_color, background_color);
    }
}

void __Win32PrintError(EConsoleColor text_color, EConsoleColor background_color, char *fmt, va_list args)
{
    char *message = nullptr;
    int chars_read = 1 + __Win32FormatString(message, 1, fmt, args);
    message = talloc<char>(chars_read);
    __Win32FormatString(message, chars_read, fmt, args);
    
    if (IsDebuggerPresent())
    {
        OutputDebugStringA(message);
    }
    else
    {
        // Otherwise, output to stderr.
        local_persist Win32StandardStream stream = Win32GetStandardStream(STD_ERROR_HANDLE);
        Win32PrintToStream(message, stream, text_color, background_color);
    }
}

i32 Win32FormatString(char *buff, i32 len, char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    int chars_read = __Win32FormatString(buff, len, fmt, list);
    va_end(list);
    
    return chars_read;
}

void Win32PrintMessage(EConsoleColor text_color, EConsoleColor background_color, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    __Win32PrintMessage(text_color, background_color, fmt, args);
    va_end(args);
}

void Win32PrintError(EConsoleColor text_color, EConsoleColor background_color, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    __Win32PrintError(text_color, background_color, fmt, args);
    va_end(args);
}

inline void mprint(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    __Win32PrintMessage(EConsoleColor::White, EConsoleColor::DarkGrey, fmt, args);
    va_end(args);
}

inline void mprinte(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    __Win32PrintError(EConsoleColor::Red, EConsoleColor::DarkGrey, fmt, args);
    va_end(args);
}

//~ File I/O

// Takes a path relative to the executable, and normalizes it into a full path.
// Tries to handle malformed input, but returns an empty string if unsuccessful.
jstring Win32NormalizePath(const char* path)
{
    // If the string is null or has length < 2, just return an empty one.
    if (!path || !path[0] || !path[1]) return {};
    
    // Start with our relative path appended to the full executable path.
    jstring exe_path = Win32GetExeFilepath();
    jstring result = exe_path + path;
    
    // Swap any back slashes for forward slashes.
    for (u32 i = 0; i < (u32)result.len; ++i) if (result[i] == '\\') result[i] = '/';
    
    // Strip double separators.
    for (u32 i = 0; i < (u32)result.len - 1; ++i)
    {
        if (result[i] == '/' && result[i + 1] == '/')
        {
            for (u32 j = i; j < (u32)result.len; ++j) result[j] = result[j + 1];
            --result.len;
            --i;
        }
    }
    
    // Evaluate any relative specifiers (./).
    if (result[0] == '.' && result[1] == '/')
    {
        for (u32 i = 0; i < (u32)result.len - 1; ++i) result[i] = result[i + 2];
        result.len -= 2;
    }
    for (u32 i = 0; i < (u32)result.len - 1; ++i)
    {
        if (result[i] != '.' && result[i + 1] == '.' && result[i + 2] == '/')
        {
            for (u32 j = i + 1; result[j + 1]; ++j) result[j] = result[j + 2];
            result.len -= 2;
        }
    }
    
    // Evaluate any parent specifiers (../).
    u32 last_separator = 0;
    for (u32 i = 0; (i < (u32)result.len - 1); ++i)
    {
        if (result[i] == '.' && result[i + 1] == '.' && result[i + 2] == '/')
        {
            u32 base = i + 2;
            u32 count = result.len - base;
            
            for (u32 j = 0; j <= count; ++j)
            {
                result[last_separator + j] = result[base + j];
            }
            
            result.len -= base - last_separator;
            i = last_separator;
            
            if (i > 0)
            {
                bool has_separator = false;
                for (i32 j = last_separator - 1; j >= 0; --j)
                {
                    if (result[j] == '/')
                    {
                        last_separator = j;
                        has_separator = true;
                        break;
                    }
                }
                if (!has_separator) return {};
            }
        }
        if (i > 0 && result[i - 1] == '/') last_separator = i - 1;
    }
    
    // Strip any leading or trailing separators.
    // NOTE(Dustin): Not sure i want this to occur
    /*
    if (result[0] == '/')
    {
        for (u32 i = 0; i < (u32)result.len; ++i) result[i] = result[i + 1];
        --result.len;
    }
    
    if (result[result.len - 1] == '/')
    {
        result[result.len - 1] = '\0';
        --result.len;
    }
    */
    
    exe_path.Clear();
    return result;
}

DynamicArray<jstring> Win32FindAllFilesInDirectory(jstring &directory, jstring delimiter)
{
    DynamicArray<jstring> files = DynamicArray<jstring>();
    
    jstring abs_path = Win32NormalizePath(directory.GetCStr());
    jstring path =  abs_path + delimiter;
    
    WIN32_FIND_DATA fd;
    HANDLE hfind = FindFirstFile(path.GetCStr(), &fd);
    
    if (hfind != INVALID_HANDLE_VALUE)
    {
        do
        {
            // read all files in current folder
            // ignore directories
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                jstring file = InitJString(fd.cFileName);
                files.PushBack(file);
            }
        } while (FindNextFile(hfind, &fd));
        
        FindClose(hfind);
    }
    
    abs_path.Clear();
    path.Clear();
    
    return files;
}

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
jstring Win32GetExeFilepath()
{
    // Get the full filepath
    char exe[WIN32_STATE_FILE_NAME_COUNT];
    size_t filename_size = sizeof(exe);
    
    DWORD filepath_size = GetModuleFileNameA(0, exe, filename_size);
    
    // find last "\"
    char *pos = 0;
    for (char *Scanner = exe; *Scanner; ++Scanner)
    {
        if (*Scanner == '\\')
        {
            *Scanner = '/'; // normalize the slash to be unix style
            pos = Scanner + 1;
        }
    }
    
    int len = (pos - exe) + 1;
    jstring result = InitJString(exe, (pos - exe));
    
    return result;
}

void Win32DetermineFileEncoding(HANDLE handle)
{
    Win32StandardStream result = {};
    
    DWORD type = GetFileType(result.handle) & (~FILE_TYPE_REMOTE);
    DWORD dummy;
    result.is_redirected = (type == FILE_TYPE_CHAR) ? !GetConsoleMode(result.handle, &dummy) : true;
    if (type == FILE_TYPE_DISK)
    {
        LARGE_INTEGER file_size;
        GetFileSizeEx(result.handle, &file_size);
        if (file_size.QuadPart > 1)
        {
            u16 bom = 0;
            SetFilePointerEx(result.handle, {}, 0, FILE_BEGIN);
            ReadFile(result.handle, &bom, 2, &dummy, 0);
            SetFilePointerEx(result.handle, {}, 0, FILE_END);
            result.is_wide = (bom == (u16)0xfeff || bom == (u16)0xfffe);
            result.is_little_endian = (bom == (u16)0xfffe);
        }
    }
    else if (type == FILE_TYPE_UNKNOWN)
    {
        DisplayError(TEXT("GetFileType"));
    }
}

HANDLE Win32OpenFile(const char *Filename, bool IsWrite)
{
    jstring AbsPath = Win32NormalizePath(Filename);
    
    HANDLE Result;
    
    if (IsWrite)
    {
        // first attempt to truncate an existing file
        Result = CreateFileA(AbsPath.GetCStr(), GENERIC_WRITE, 0, 0,
                             TRUNCATE_EXISTING,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
        
        // if above fails, just create the file
        if (Result == INVALID_HANDLE_VALUE)
        {
            Result = CreateFileA(AbsPath.GetCStr(), GENERIC_WRITE, 0, 0, CREATE_NEW,
                                 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
        }
    }
    else
    {
        Result = CreateFileA(AbsPath.GetCStr(),
                             GENERIC_READ,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
    }
    
    if (Result == INVALID_HANDLE_VALUE)
    {
        DisplayError(TEXT("OpenFile"));
        CloseHandle(Result);
    }
    
    AbsPath.Clear();
    
    return Result;
}

void Win32WriteToFile(HANDLE FileHandle, char *Fmt, ...)
{
    va_list Args;
    va_start(Args, Fmt);
    
    char *Message = nullptr;
    int CharsRead = 1 + __Win32FormatString(Message, 1, Fmt, Args);
    Message = palloc<char>(CharsRead);
    __Win32FormatString(Message, CharsRead, Fmt, Args);
    
    DWORD BytesWritten = 0;
    BOOL err = WriteFile(FileHandle,         // open file handle
                         Message,            // start of data to write
                         CharsRead - 1,      // number of bytes to write
                         &BytesWritten,      // number of bytes that were written
                         NULL);              // no overlapped structure
    
    if (err == FALSE)
    {
        DisplayError(TEXT("WriteFile"));
        mprinte("Terminal failure: Unable to write to file.\n");
    }
    else
    {
        if (BytesWritten != CharsRead - 1)
        {
            // This is an error because a synchronous write that results in
            // success (WriteFile returns TRUE) should write all data as
            // requested. This would not necessarily be the case for
            // asynchronous writes.
            mprinte("Error: dwBytesWritten != dwBytesToWrite\n");
        }
    }
    
    va_end(Args);
}

void Win32CloseFile(HANDLE FileHandle)
{
    CloseHandle(FileHandle);
}

jstring Win32LoadFile(jstring &filename)
{
    jstring abs_path = Win32NormalizePath(filename.GetCStr());
    
    HANDLE handle = CreateFileA(abs_path.GetCStr(),
                                GENERIC_READ,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    
    if (handle == INVALID_HANDLE_VALUE)
    {
        DisplayError(TEXT("CreateFile"));
        CloseHandle(handle);
        
        abs_path.Clear();
        
        jstring result = {};
        return result;
    }
    
    // retrieve the file information the
    BY_HANDLE_FILE_INFORMATION file_info;
    // NOTE(Dustin): This can also be used to query the last time the file has been written to
    BOOL err = GetFileInformationByHandle(handle, &file_info);
    
    if (!err)
    {
        DisplayError(TEXT("GetFileInformationByHandle"));
        CloseHandle(handle);
        
        abs_path.Clear();
        
        jstring result = {};
        return result;
    }
    
    // size of a DWORD is always 32 bits
    // NOTE(Dustin): Will there be a case where the file size will
    // be greater than 2^32? If so, might want to stream the data.
    // ReadFile max size seems to be 2^32.
    DWORD low_size  = file_info.nFileSizeLow;
    DWORD high_size = file_info.nFileSizeHigh;
    
    // NOTE(Dustin): Instead of OR'ing the values together, could probably
    // just check to see if high_size is greater than 0
    u64 file_size = (((u64)high_size)<<32) | (((u64)low_size)<<0);
    if (file_size >= ((u64)1<<32))
    {
        mprinte("File \"%s\" is too large and should probably be streamed from disc!\n", abs_path.GetCStr());
        CloseHandle(handle);
        
        abs_path.Clear();
        
        jstring result = {};
        return result;
    }
    
    jstring result = InitJString(file_size);
    
    DWORD bytes_read = 0;
    err = ReadFile(handle,
                   (result.heap) ? result.hptr : result.sptr,
                   low_size,
                   &bytes_read,
                   NULL);
    
    if (err == 0)
    {
        // NOTE(Dustin): Note that 0 can be returned if the
        // read operating is occuring asynchronously
        DisplayError(TEXT("ReadFile"));
        CloseHandle(handle);
        
        abs_path.Clear();
        
        jstring result = {};
        return result;
    }
    
    result.len = bytes_read;
    if (result.heap)
    {
        result.hptr[bytes_read] = 0;
    }
    else
    {
        result.sptr[bytes_read] = 0;
    }
    CloseHandle(handle);
    
    abs_path.Clear();
    
    return result;
}

// TODO(Dustin): Allow for appending to a file
void Win32WriteBufferToFile(jstring &file, void *buffer, u32 size)
{
    jstring abs_path = Win32NormalizePath(file.GetCStr());
    HANDLE handle = CreateFileA(abs_path.GetCStr(), GENERIC_WRITE, 0, 0,
                                TRUNCATE_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
    
    if (handle == INVALID_HANDLE_VALUE)
    {
        handle = CreateFileA(abs_path.GetCStr(), GENERIC_WRITE, 0, 0, CREATE_NEW,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
    }
    
    if (handle == INVALID_HANDLE_VALUE)
    {
        DisplayError(TEXT("CreateFile"));
        _tprintf(TEXT("Terminal failure: Unable to open file \"%s\" for write.\n"), abs_path.GetCStr());
    }
    else
    {
        DWORD bytes_written = 0;
        BOOL err = WriteFile(handle,         // open file handle
                             buffer,         // start of data to write
                             size,           // number of bytes to write
                             &bytes_written, // number of bytes that were written
                             NULL);          // no overlapped structure
        
        if (err == FALSE)
        {
            DisplayError(TEXT("WriteFile"));
            mprinte("Terminal failure: Unable to write to file.\n");
        }
        else
        {
            if (bytes_written != size)
            {
                // This is an error because a synchronous write that results in
                // success (WriteFile returns TRUE) should write all data as
                // requested. This would not necessarily be the case for
                // asynchronous writes.
                mprinte("Error: dwBytesWritten != dwBytesToWrite\n");
            }
        }
        
    }
    
    abs_path.Clear();
    CloseHandle(handle);
}

void Win32DeleteFile(jstring &file)
{
    jstring abs_path = Win32NormalizePath(file.GetCStr());
    
    if (!DeleteFile(abs_path.GetCStr()))
    {
        DisplayError(TEXT("DeleteFile"));
    }
    
    abs_path.Clear();
}

void Win32ExecuteCommand(jstring &system_cmd)
{
    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi;
    si.cb = sizeof(si);
    
    LPSTR input = (LPSTR)system_cmd.GetCStr();
    
    DWORD err;
    if (!CreateProcess(NULL, input, NULL, NULL, FALSE,
                       0, 0, 0, &si, &pi)) {
        DisplayError(TEXT("CreateProcess"));
    }
    else
    {
        // Wait till process completes
        WaitForSingleObject(pi.hProcess, INFINITE);
        // Check process’s exit code
        GetExitCodeProcess(pi.hProcess, &err);
        // Avoid memory leak by closing process handle
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

file_internal void Win32ShutdownRoutines()
{
    mm::ShutdownMemoryManager();
}

void* Win32RequestMemory(u64 Size)
{
    SYSTEM_INFO sSysInfo;
    DWORD       dwPageSize;
    LPVOID      lpvBase;
    u64         ActualSize;
    
    GetSystemInfo(&sSysInfo);
    dwPageSize = sSysInfo.dwPageSize;
    
    ActualSize = (Size + (u64)dwPageSize - 1) & ~((u64)dwPageSize - 1);
    
    lpvBase = VirtualAlloc(NULL,                    // System selects address
                           ActualSize,              // Size of allocation
                           MEM_COMMIT|MEM_RESERVE,  // Allocate reserved pages
                           PAGE_NOACCESS);          // Protection = no access
    
    return lpvBase;
}

void Win32ReleaseMemory(void *Ptr)
{
    BOOL bSuccess = VirtualFree(Ptr,           // Base address of block
                                0,             // Bytes of committed pages
                                MEM_RELEASE);  // Decommit the pages
    assert(bSuccess && "Unable to free a VirtualAlloc allocation!");
}

// Arg 1 should be the input material file
// Arg 2 should be the output reflection file
int main(int argc, char *argv[])
{
    // Initialize Management Routines
    mm::InitializeMemoryManager(_MB(5), _MB(1));
	
	// only arg that is really important in #1, which is the config file
	//assert(argc == 2);
    for (i32 i = 0; i < argc; ++i)
		mprinte("Argument %d:\t%s\n", i, argv[i]);
	
    jstring ConfigFile = pstring(argv[1]);
    jstring ReflFile = pstring(argv[2]);
    
    config_obj_table ConfigTable = LoadConfigFile(ConfigFile);
    
    config_obj ShaderObj = GetConfigObj(&ConfigTable, "Shaders");
    
    jstring Vert = GetConfigStr(&ShaderObj, "Vertex");
    jstring Frag = GetConfigStr(&ShaderObj, "Fragment");
    
    shader VertShader = { Vert, Shader_Vertex   };
    shader FragShader = { Frag, Shader_Fragment };
    
    mprinte("Name of the vertex file: %s\n", Vert.GetCStr());
    mprinte("Name of the fragment file: %s\n", Frag.GetCStr());
    
    DynamicArray<shader> ShaderList = DynamicArray<shader>(2);
    ShaderList.PushBack(VertShader);
    ShaderList.PushBack(FragShader);
    
    GenerateReflectionInfo(ShaderList, ReflFile);
    
    Vert.Clear();
    Frag.Clear();
    ReflFile.Clear();
    ShaderList.Reset();
    FreeConfigObjTable(&ConfigTable);
    
    Win32ShutdownRoutines();
    return (0);
}