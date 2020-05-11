#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <Tchar.h>
#include <strsafe.h>
#include <stdio.h>

#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_win32.cpp"

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 512
#endif

// Timing information
file_global i64 GlobalPerfCountFrequency;

// Window information
file_global HWND ClientWindow;
file_global RECT ClientWindowRect    = {};
file_global RECT ClientWindowRectOld = ClientWindowRect;

file_global u32 ClientWindowWidth;
file_global u32 ClientWindowHeight;

// Mouse information
file_global r32 GlobalMouseXPos;
file_global r32 GlobalMouseYPos;

// Game or Dev Mode?
file_global bool GlobalIsDevMode = true;
file_global bool GlobalIsFullscreen = false;

file_global bool ClientIsRunning = false;

file_global FrameInput GlobalFrameInput;

struct CodeDLL
{
    HMODULE game;
} GlobalCodeDLL;

ErrorCode SetError;
char ErrorBuffer[512];

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// TODO(Dustin): Logging has a bug when printing standard text to a command
// prompt. Error printing works just fine, but printing to STD_OUTPUT_HANDLE
// produces no output. This only occurs with command prompt. So far, all other
// consoles have the expected output. This might be fixed.

// TODO(Dustin): Logging needs to be better handled. The main missing functionality
// is being able to flush the log console to a file. The two required pieces of
// functionality are:
// 1. At program exit flush the log console to a file
// 2. Detect when the console reaches the max allowed lines and flush the lines about
//    to be overwritten to a file.

// TODO(Dustin): Adapt FileBuffer functionality to handle TEXT based writes. Currently
// the FileBuffer only supports binary output, which is rather inconvient. Furthermore,
// writing to the Binary file is rather inconvientent - requiring unqiue helper functions
// for every type to write. Need to compress the current approach to make development and
// maintenance easier.
//
// Two potential options:
// 1. Use snprintf (or other thread safe version)
// 2. Implement my own snptrinf function
//   - Potentially could handle binary vs TEXT in a unique implementation?
//
// A possible extension of this idea is to implement a wrapper around winapi file
// descriptors. this could allow for a user to specify if the write operations should
// be binary for TEXT. Any subsequent calls to write could handle the conversion internally.
//
// This idea has sort of been implemented with the PlatformFormatString function. Just needs
// to be tested.


// TODO(Dustin): Allow for mouse capture. This will allow for third/first person camera to
// be implemented in the client layer.


//~ Error Handling

// TODO(Dustin): Finish implementing - Need logging working appropriately
// NOTE(Dustin): Exception handling -
// This does not currently support async models.
// the application does not exit gracefully! The purpose of
// this function to write an error to the log file BEFORE
// exiting. For future Async I/O will have to call CancelIoEx
// and then wait for an outgoing IO requeststo cancel before
// writing and closing the Logging handle.

void Win32RaiseError(ErrorCode error, char *fmt, ...)
{
    SetError = error;
    
    
}


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
    jstring result = Win32GetExeFilepath() + path;
    
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
        DisplayError(TEXT("CreateFile"));
        
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

//~
// NOTE(Dustin): Keep? Depends on how I end up setting up the
// Engine - Game toolchain
#if 0
GameApi LoadGameApi(const char *libname)
{
    GameApi gc = {};
    
    GlobalCodeDLL.game = LoadLibrary(TEXT(libname));
    
    if (GlobalCodeDLL.game != NULL)
    {
        gc.Initialize =
            (game_initialize *)GetProcAddress(GlobalCodeDLL.game, "GameInitialize");
        gc.Update     = (game_update   *)GetProcAddress(GlobalCodeDLL.game, "GameUpdate");
        gc.Resize     = (game_resize   *)GetProcAddress(GlobalCodeDLL.game, "GameResize");
        gc.Shutdown   = (game_shutdown *)GetProcAddress(GlobalCodeDLL.game, "GameShutdown");
        if (gc.Initialize && gc.Update && gc.Resize && gc.Shutdown)
        {
            gc.is_valid = true;
        }
    }
    
    return gc;
}
#endif

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

void Win32WaitForEvents()
{
    // pause while the app is de-iconified
}

u64 Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return (Result.QuadPart);
    
}

r32 Win32GetSecondsElapsed(u64 start, u64 end)
{
    r32 Result = ((r32)(end - start) /
                  (r32)GlobalPerfCountFrequency);
    return(Result);
}

// TODO(Dustin): Collect all required extensions for a platform
// requires that a Dynamic Array is returned
const char* Win32GetRequiredInstanceExtensions(bool validation_layers)
{
    VkResult err;
    VkExtensionProperties* ep;
    u32 count = 0;
    err = vk::vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
    if (err)
    {
        mprinte("Error enumerating Instance extension properties!\n");
    }
    
    ep = palloc<VkExtensionProperties>();
    err = vk::vkEnumerateInstanceExtensionProperties(NULL, &count, ep);
    if (err)
    {
        mprinte("Unable to retrieve enumerated extension properties!\n");
        count = 0;
    }
    
    for (u32 i = 0;  i < count;  i++)
    {
        if (strcmp(ep[i].extensionName, "VK_KHR_win32_surface") == 0)
        {
            const char *plat_exts = "VK_KHR_win32_surface";
            return plat_exts;
        }
    }
    
    mprinte("Could not find win32 vulkan surface extension!\n");
    return "";
};

void Win32VulkanCreateSurface(VkSurfaceKHR *surface, VkInstance vulkan_instance)
{
    VkWin32SurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hinstance = GetModuleHandle(NULL);;
    surface_info.hwnd      = ClientWindow;
    
    VK_CHECK_RESULT(vk::vkCreateWin32SurfaceKHR(vulkan_instance, &surface_info, nullptr, surface),
                    "Unable to create XCB Surface!\n");
}

void Win32GetClientWindowDimensions(u32 *width, u32 *height)
{
    RECT rect;
    GetClientRect(ClientWindow, &rect);
    
    *width  = rect.right - rect.left;
    *height = rect.bottom - rect.top;
}

file_internal void Win32ShutdownRoutines()
{
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    event::ManagerShutdown();
    vk::ShutdownVulkan();
    ecs::ShutdownECS();
    mm::ShutdownMemoryManager();
}

struct TestEvent
{
    int y = 0;
    int x = 10;
};

file_internal void TestFnctPtr(void *instance, TestEvent event)
{
    mprinte("Function pointer was called!\n");
}

struct TestEventCallback
{
    int a = 2;
    int b = 1;
} test_callback;

file_internal void TestFnctPtrWithInstance(void *instance, TestEvent event)
{
    TestEventCallback *callback_instance = (TestEventCallback*)instance;
    
    mprinte("Function pointer with callback was called! a: %d, b: %d\n", callback_instance->a, callback_instance->b);
}

file_internal void WindowResizeEventCallback(void *instance, WindowResizeEvent event)
{
    CoreVulkanResizeEvent cv_event;
    cv_event.Width  = event.Width;
    cv_event.Height = event.Height;
    
    event::Dispatch<CoreVulkanResizeEvent>(cv_event);
}

file_internal void CoreVulkanResizeEventCallback(void *instance, CoreVulkanResizeEvent event)
{
    // Idle <- wait for last frame to finish rendering
    vk::Idle();
    vk::Resize();
    
    ResizeEvent r_event;
    r_event.Width  = event.Width;
    r_event.Height = event.Height;
    
    event::Dispatch<ResizeEvent>(r_event);
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR lpCmdLine, INT nCmdShow)
{
    //~ Timing information
    // Setup timing information
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    
    UINT desired_scheduler_ms = 1;
    bool sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
    
    //~ Create Client Window
    
    ClientWindowWidth = 1080;
    ClientWindowHeight = 720;
    
    const char CLASS_NAME[] = "Maple Window Class";
    
    WNDCLASS wc = {0};
    wc.style = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);
    
    ClientWindow = CreateWindowEx(0,
                                  CLASS_NAME,
                                  APP_NAME,
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  ClientWindowWidth,
                                  ClientWindowHeight,
                                  NULL,
                                  NULL,
                                  hInstance,
                                  NULL);
    
    if (ClientWindow == NULL) return 0;
    
    GetClientRect(ClientWindow, &ClientWindowRect);
    ClientWindowRectOld = ClientWindowRect;
    
    //~ Initialize routines
    
    // Initialize Management Routines
    // NOTE(Dustin): Macros ar defined in engine_config.h
    mm::InitializeMemoryManager(MEMORY_USAGE, TRANSIENT_MEMORY);
    ecs::InitializeECS();
    
    // Enable graphics
    if (!vk::InitializeVulkan())
    {
        mprinte("Unable to initialize Vulkan!\n");
        //glfwTerminate();
        ecs::ShutdownECS();
        mm::ShutdownMemoryManager();
        
        exit(1);
    }
    
    //~ Event Initialization
    event::ManagerInit();
    
    event::Subscribe<WindowResizeEvent>(&WindowResizeEventCallback, nullptr);
    event::Subscribe<CoreVulkanResizeEvent>(&CoreVulkanResizeEventCallback, nullptr);
    
    //~ Initialize ImGui information
    
    ImGuiContext *ctx = ImGui::CreateContext();
    if (!ImGui_ImplWin32_Init(ClientWindow))
    {
        mprinte("Unable to initialize Win32 ImGui!\n");
        vk::ShutdownVulkan();
        ecs::ShutdownECS();
        mm::ShutdownMemoryManager();
        
        exit(1);
    }
    
    //~ Client Initialization
    GameInit();
    
    //~ App Loop
    ::ShowWindow(ClientWindow, nCmdShow);
    
    u64 last_frame_time = Win32GetWallClock();
    r32 refresh_rate = 60.0f;
    r32 target_seconds_per_frame = 1 / refresh_rate;
    
    GlobalFrameInput = {0};;
    ClientIsRunning = true;
    MSG msg = {};
    while (ClientIsRunning)
    {
        bool last_frame_wireframe = GlobalFrameInput.RenderWireframe;
        GlobalFrameInput = {0};
        GlobalFrameInput.RenderWireframe = last_frame_wireframe;
        
        // Message loop
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        u64 clock_now = Win32GetWallClock();
        r32 seconds_elapsed_update = Win32GetSecondsElapsed(last_frame_time, clock_now);
        
        r32 seconds_elapsed_per_frame = seconds_elapsed_update;
        if (seconds_elapsed_per_frame < target_seconds_per_frame)
        {
            if (sleep_is_granular)
            {
                DWORD sleep_ms = (DWORD)(1000.0f * (target_seconds_per_frame - seconds_elapsed_per_frame));
                if (sleep_ms > 0)
                {
                    Sleep(sleep_ms);
                }
                
                seconds_elapsed_per_frame = Win32GetSecondsElapsed(last_frame_time, Win32GetWallClock());
            }
        }
        else
        {
            // LOG: Missed frame rate!
        }
        
        last_frame_time = Win32GetWallClock();
        
        // TODO(Dustin): Handle input
        GlobalFrameInput.TimeElapsed = seconds_elapsed_per_frame;
        GameUpdateAndRender(GlobalFrameInput);
    }
    
    GameShutdown();
    Win32ShutdownRoutines();
    
    return (0);
}

file_internal void SetFullscreen(bool fullscreen)
{
    if (GlobalIsFullscreen != fullscreen)
    {
        GlobalIsFullscreen = fullscreen;
        
        if (GlobalIsFullscreen)
        {
            ::GetWindowRect(ClientWindow, &ClientWindowRect);
            
            UINT window_style = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION |
                                                        WS_SYSMENU |
                                                        WS_THICKFRAME |
                                                        WS_MINIMIZEBOX |
                                                        WS_MAXIMIZEBOX);
            ::SetWindowLong(ClientWindow, GWL_STYLE, window_style);
            
            HMONITOR hmonitor = ::MonitorFromWindow(ClientWindow, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitor_info = {};
            ::GetMonitorInfo(hmonitor, &monitor_info);
            
            ::SetWindowPos(ClientWindow, HWND_TOP,
                           monitor_info.rcMonitor.left,
                           monitor_info.rcMonitor.top,
                           monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                           monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                           SWP_FRAMECHANGED | SWP_NOACTIVATE);
            
            ::ShowWindow(ClientWindow, SW_MAXIMIZE);
        }
        else
        {
            ::SetWindowLong(ClientWindow, GWL_STYLE, WS_OVERLAPPEDWINDOW);
            
            ::SetWindowPos(ClientWindow, HWND_NOTOPMOST,
                           ClientWindowRectOld.left,
                           ClientWindowRectOld.top,
                           ClientWindowRectOld.right - ClientWindowRectOld.left,
                           ClientWindowRectOld.bottom - ClientWindowRectOld.top,
                           SWP_FRAMECHANGED | SWP_NOACTIVATE);
            
            ::ShowWindow(ClientWindow, SW_NORMAL);
        }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // messages can get sent before window has finished initialize and after WM_Close
    // has be sent...
    if (!ClientIsRunning)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    
    // Update ImGui
    ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);
    
    switch (uMsg)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            ClientIsRunning = false;
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(ClientWindow, &Paint);
            EndPaint(ClientWindow, &Paint);
        } break;
        
        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
        {
            //printf("Mouse button pressed in main win32!\n");
        } break;
        
        //if (!io.WantCaptureKeyboard)
        {
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            {
                bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                switch (wParam)
                {
                    case VK_UP:
                    {
                        GlobalFrameInput.Keyboard.KEY_ARROW_UP = 1;
                    } break;
                    
                    case VK_DOWN:
                    {
                        GlobalFrameInput.Keyboard.KEY_ARROW_DOWN = 1;
                    } break;
                    
                    case VK_LEFT:
                    {
                        GlobalFrameInput.Keyboard.KEY_ARROW_LEFT = 1;
                    } break;
                    
                    case VK_RIGHT:
                    {
                        GlobalFrameInput.Keyboard.KEY_ARROW_RIGHT = 1;
                    } break;
                    
                    case VK_F5:
                    {
                        GlobalFrameInput.RenderWireframe = !GlobalFrameInput.RenderWireframe;
                    } break;
                    
                    case 'W':
                    {
                        GlobalFrameInput.Keyboard.KEY_W = 1;
                    } break;
                    
                    case 'S':
                    {
                        GlobalFrameInput.Keyboard.KEY_S = 1;
                    } break;
                    
                    case 'A':
                    {
                        GlobalFrameInput.Keyboard.KEY_A = 1;
                    } break;
                    
                    case 'D':
                    {
                        GlobalFrameInput.Keyboard.KEY_D = 1;
                    } break;
                    
                    case VK_ESCAPE:
                    {
                        ClientIsRunning = false;
                    } break;
                    case VK_RETURN:
                    {
                        if (alt)
                        {
                            case VK_F11:
                            {
                                SetFullscreen(!GlobalIsFullscreen);
                            } break;
                        }
                    } break;
                    
                    default: break;
                }
            } break;
        }
        
        case WM_SYSCHAR: break;
        
        case WM_SIZE:
        {
            ClientWindowRectOld = ClientWindowRect;
            GetClientRect(hwnd, &ClientWindowRect);
            
            int width = ClientWindowRect.right - ClientWindowRect.left;
            int height = ClientWindowRect.bottom - ClientWindowRect.top;
            
            WindowResizeEvent event;
            event.Width  = (width >= 1) ? width : 1;
            event.Height = (height >= 1) ? height : 1;
            
            event::Dispatch<WindowResizeEvent>(event);
        } break;
        
        default: break;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
