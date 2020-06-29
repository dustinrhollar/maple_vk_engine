
// TODO(Dustin):
// Build script - change name of pdb file so that hot reloading works with VS debugger
// Loop editor, useful for fine tuning gameplay mechanics, HH Day 23 is a good walkthrough
// Profiling Tools
// Debug Tools
// Audio

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <Tchar.h>
#include <strsafe.h>

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 512
#endif

struct platform_window
{
    HWND Window;
};

// Timing information
file_global i64 GlobalPerfCountFrequency;

// Window information
file_global HWND ClientWindow;
file_global RECT ClientWindowRect;
file_global RECT ClientWindowRectOld;
file_global bool GlobalIsFullscreen  = false;
file_global bool ClientIsRunning     = false;

// Mouse information
file_global r32 GlobalMouseXPos;
file_global r32 GlobalMouseYPos;

// Game or Dev Mode?
file_global bool GlobalIsDevMode = true;
file_global bool RenderDevGui    = true;

// Frame Info
file_global u64 FrameCount = 0;

// Global Memory Handles
file_global void             *GlobalMemoryPtr;
file_global free_allocator    PermanantMemory;
file_global tagged_heap       TaggedHeap;
file_global tag_id_t          PlatformTag = {0, TAG_ID_PLATFORM, 0};
file_global tagged_heap_block PlatformHeap;

// graphics state
renderer_t Renderer;
resource_registry ResourceRegistry;
asset_registry AssetRegistry;

// File I/O Handling
struct file
{
    HANDLE      Handle;
    
    // The current size of the file.
    u64         FileSize;
    
    // Two behaviors for backed memory of a file:
    // 1. Write operations: When writing to a file, HeapBlock will be
    //    used. When the tagged block is filled, it is flushed to a file.
    // 2. Read operations: If the file size is greater than the allowed block
    //    size for the tagged heap, then the memory is allocated from system memory.
    //    Otherise, use the TaggedHeap as a linear allocator.
    union
    {
        struct
        {
            tag_id_t          FileTag;
            tagged_heap_block Block;  // heap memory
        };
        
        void                 *Memory; // platform memory
    };
};


struct game_code
{
    HMODULE Handle;
    FILETIME DllLastWriteTime;
    
    game_stage_entry *GameStageEntry;
};


#define MAX_OPEN_FILES 10
file_global file OpenFiles[MAX_OPEN_FILES];
file_global u32 NextFileId = 0;

file_internal void SetFullscreen(bool fullscreen);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
mstring Win32GetExeFilepath();
mstring Win32NormalizePath(const char* path);

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

// An internal allocation scheme that allocates from the 2MB Platform Linear allcoator.
// this functions is primarily used by print/formatting functions that need temporary,
// dynamic memory. When the heap is filled, the allocator is reset.
file_internal void* PlatformLocalAlloc(u32 Size)
{
    void* Result = TaggedHeapBlockAlloc(&PlatformHeap, Size);
    if (!Result)
    {
        PlatformHeap.Brkp = PlatformHeap.Start;
        
        Result = TaggedHeapBlockAlloc(&PlatformHeap, Size);
        if (!Result)
            mprinte("Error allocating from Platform Heap Allocator!\n");
    }
    
    return Result;
}

//~ Game Code Hot loading

GAME_STAGE_ENTRY(GameStageEntryStub)
{
}

inline FILETIME Win32GetLastWriteTime(const char *Filename)
{
    FILETIME LastWriteTime = {};
    
    BY_HANDLE_FILE_INFORMATION FileInfo;
    
    BOOL Err = GetFileAttributesEx(Filename,
                                   GetFileExInfoStandard,
                                   &FileInfo);
    
    if (Err)
        LastWriteTime = FileInfo.ftLastWriteTime;
    
    return LastWriteTime;
}

void Win32LoadGameCode(game_code *GameCode, const char *GameDllName)
{
    mstring GameDllCopy = Win32NormalizePath("game_temp.dll");
    
    GameCode->DllLastWriteTime = Win32GetLastWriteTime(GameDllName);
    
    CopyFile(GameDllName, GetStr(&GameDllCopy), FALSE);
    
    GameCode->Handle = LoadLibrary(GetStr(&GameDllCopy));
    GameCode->GameStageEntry = NULL;
    
    if (GameCode->Handle)
    {
        GameCode->GameStageEntry = (game_stage_entry*)GetProcAddress(GameCode->Handle, "GameStageEntry");
    }
    else
    {
        mprinte("Unable to load game dll!\n");
    }
    
    if (!GameCode->GameStageEntry)
    {
        mprinte("Unable to find proc address for \"GameStageEntry\"!\n");
        GameCode->GameStageEntry = &GameStageEntryStub;
    }
    
    MstringFree(&GameDllCopy);
}

void Win32UnloadGameCode(game_code *GameCode)
{
    if (GameCode->Handle)
        FreeLibrary(GameCode->Handle);
    GameCode->Handle = 0;
    GameCode->GameStageEntry = &GameStageEntryStub;
}

//~ File I/O

// Takes a path relative to the executable, and normalizes it into a full path.
// Tries to handle malformed input, but returns an empty string if unsuccessful.
mstring Win32NormalizePath(const char* path)
{
    // If the string is null or has length < 2, just return an empty one.
    if (!path || !path[0] || !path[1]) return {};
    
    // Start with our relative path appended to the full executable path.
    mstring exe_path = Win32GetExeFilepath();
    mstring result = {0};
    MstringAdd(&result, &exe_path, path, strlen(path));
    
    char *Str = GetStr(&result);
    
    // Swap any back slashes for forward slashes.
    for (u32 i = 0; i < (u32)result.Len; ++i) if (Str[i] == '\\') Str[i] = '/';
    
    // Strip double separators.
    for (u32 i = 0; i < (u32)result.Len - 1; ++i)
    {
        if (Str[i] == '/' && Str[i + 1] == '/')
        {
            for (u32 j = i; j < (u32)result.Len; ++j) Str[j] = Str[j + 1];
            --result.Len;
            --i;
        }
    }
    
    // Evaluate any relative specifiers (./).
    if (Str[0] == '.' && Str[1] == '/')
    {
        for (u32 i = 0; i < (u32)result.Len - 1; ++i) Str[i] = Str[i + 2];
        result.Len -= 2;
    }
    for (u32 i = 0; i < (u32)result.Len - 1; ++i)
    {
        if (Str[i] != '.' && Str[i + 1] == '.' && Str[i + 2] == '/')
        {
            for (u32 j = i + 1; Str[j + 1]; ++j) Str[j] = Str[j + 2];
            result.Len -= 2;
        }
    }
    
    // Evaluate any parent specifiers (../).
    u32 last_separator = 0;
    for (u32 i = 0; (i < (u32)result.Len - 1); ++i)
    {
        if (Str[i] == '.' && Str[i + 1] == '.' && Str[i + 2] == '/')
        {
            u32 base = i + 2;
            u32 count = result.Len - base;
            
            for (u32 j = 0; j <= count; ++j)
            {
                Str[last_separator + j] = Str[base + j];
            }
            
            result.Len -= base - last_separator;
            i = last_separator;
            
            if (i > 0)
            {
                bool has_separator = false;
                for (i32 j = last_separator - 1; j >= 0; --j)
                {
                    if (Str[j] == '/')
                    {
                        last_separator = j;
                        has_separator = true;
                        break;
                    }
                }
                if (!has_separator) return {};
            }
        }
        if (i > 0 && Str[i - 1] == '/') last_separator = i - 1;
    }
    
    MstringFree(&exe_path);
    return result;
}

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
mstring Win32GetExeFilepath()
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
    
    mstring result;
    Mstring(&result, exe, (pos - exe));
    
    return result;
}

void PlatformCloseFile(file_t File)
{
    if (File->Handle == INVALID_HANDLE_VALUE) return;
    
    if (File->FileSize > TaggedHeap.BlockSize)
    {
        PlatformReleaseMemory(File->Memory, File->FileSize);
        File->Memory = nullptr;
    }
    else
    {
        TaggedHeapReleaseAllocation(&TaggedHeap, File->FileTag);
        
        File->Block.Start      = NULL;
        File->Block.Brkp       = NULL;
        File->Block.End        = NULL;
        File->Block.TaggedHeap = NULL;
    }
    
    CloseHandle(File->Handle);
    File->FileSize = 0;
    File->Handle   = INVALID_HANDLE_VALUE;
}

void* GetFileBuffer(file_t File)
{
    if (File->FileSize > TaggedHeap.BlockSize)
    {
        return File->Memory;
    }
    else
    {
        return File->Block.Start;
    }
}

u64 PlatformGetFileSize(file_t File)
{
    return File->FileSize;
}

file_t PlatformLoadFile(const char *Filename, bool Append)
{
    file_t Result = NULL;
    
    // Find an open file handle
    for (u32 i = 0; i < MAX_OPEN_FILES; ++i)
    {
        if (OpenFiles[i].Handle == INVALID_HANDLE_VALUE)
        {
            Result = OpenFiles + i;
            break;
        }
    }
    
    if (Result)
    {
        mstring AbsPath = Win32NormalizePath(Filename);
        
        Result->Handle = CreateFileA(GetStr(&AbsPath),
                                     GENERIC_READ,
                                     0,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL);
        
        if (Result->Handle == INVALID_HANDLE_VALUE)
        {
            DisplayError(TEXT("CreateFile"));
            MstringFree(&AbsPath);
            
            return Result;
        }
        
        // retrieve the file information the
        BY_HANDLE_FILE_INFORMATION file_info;
        // NOTE(Dustin): This can also be used to query the last time the file has been written to
        BOOL err = GetFileInformationByHandle(Result->Handle, &file_info);
        
        if (!err)
        {
            DisplayError(TEXT("GetFileInformationByHandle"));
            CloseHandle(Result->Handle);
            Result->Handle = INVALID_HANDLE_VALUE;
            MstringFree(&AbsPath);
            
            return Result;
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
            mprinte("File \"%s\" is too large and should probably be streamed from disc!\n", GetStr(&AbsPath));
            CloseHandle(Result->Handle);
            Result->Handle = INVALID_HANDLE_VALUE;
            MstringFree(&AbsPath);
            
            return Result;
        }
        
        DWORD bytes_read = 0;
        Result->FileSize = file_size;
        
        if (file_size > TaggedHeap.BlockSize)
        { // we need to allocate from platform memory
            Result->Memory = PlatformRequestMemory(file_size);
            
            err = ReadFile(Result->Handle,
                           Result->Memory,
                           low_size,
                           &bytes_read,
                           NULL);
        }
        else
        { // we can just allocate from the tag heap
            Result->FileTag = { NextFileId++, TAG_ID_FILE, 0 };
            Result->Block   = TaggedHeapRequestAllocation(&TaggedHeap, Result->FileTag);
            
            err = ReadFile(Result->Handle,
                           Result->Block.Start,
                           low_size,
                           &bytes_read,
                           NULL);
            
            Result->Block.Brkp += bytes_read;
        }
        
        
        if (err == 0)
        {
            // NOTE(Dustin): Note that 0 can be returned if the
            // read operating is occuring asynchronously
            DisplayError(TEXT("ReadFile"));
            CloseHandle(Result->Handle);
            
            MstringFree(&AbsPath);
            
            return Result;
        }
        
        MstringFree(&AbsPath);
    }
    else
    {
        mprinte("Unable to find an open file handle. Please close a file handle to open another!\n");
    }
    
    return Result;
}

file_t PlatformOpenFile(const char *Filename, bool Append)
{
    file_t Result = NULL;
    
    // Find an open file handle
    for (u32 i = 0; i < MAX_OPEN_FILES; ++i)
    {
        if (OpenFiles[i].Handle == INVALID_HANDLE_VALUE)
        {
            Result = OpenFiles + i;
            break;
        }
    }
    
    if (Result)
    {
        mstring AbsPath = Win32NormalizePath(Filename);
        
        if (Append)
        {
            Result->Handle = CreateFileA(GetStr(&AbsPath), GENERIC_WRITE, 0, 0,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
        }
        else
        {
            Result->Handle = CreateFileA(GetStr(&AbsPath), GENERIC_WRITE, 0, 0,
                                         TRUNCATE_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
        }
        
        // file doesn't currently exist
        if (Result->Handle == INVALID_HANDLE_VALUE)
        {
            Result->Handle= CreateFileA(GetStr(&AbsPath), GENERIC_WRITE, 0, 0, CREATE_NEW,
                                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
        }
        
        if (Result->Handle == INVALID_HANDLE_VALUE)
        {
            DisplayError(TEXT("CreateFile"));
            _tprintf(TEXT("Terminal failure: Unable to open file \"%s\" for write.\n"), GetStr(&AbsPath));
            
            Result = NULL;
        }
        else
        {
            Result->FileTag = { NextFileId++, TAG_ID_FILE, 0 };
            Result->Block   = TaggedHeapRequestAllocation(&TaggedHeap, Result->FileTag);
        }
        
        
        MstringFree(&AbsPath);
    }
    else
    {
        mprinte("Unable to find an open file handle. Please close a file handle to open another!\n");
    }
    
    return Result;
}

// NOTE(Dustin): Right now I am assuming all file writes are using
// the tagged block.
void PlatformFlushFile(file_t File)
{
    void *MemPtr = File->Block.Start;
    u64 FileSize = File->Block.Brkp - File->Block.Start;
    
    DWORD BytesWritten;
    BOOL err = WriteFile(File->Handle,
                         MemPtr,
                         FileSize,
                         &BytesWritten,
                         NULL);
    
    if (err == FALSE)
    {
        DisplayError(TEXT("WriteFile"));
        mprinte("Terminal failure: Unable to write to file.\n");
    }
    
    // Reset the linear allocator
    File->Block.Brkp = File->Block.Start;
}

inline void FlushBinaryWriteIfNeeded(file_t File, u32 ReqSize)
{
    if (ReqSize > TaggedHeap.BlockSize)
    {
        mprinte("Yo, binary write is over 2MB. Please avoid doing that!\n");
    }
    else
    {
        if (File->Block.Brkp + ReqSize > File->Block.End)
        {
            PlatformFlushFile(File);
        }
    }
}

void PlatformWriteBinaryStreamToFile(file_t File, void *DataPtr, u64 DataSize)
{
    PlatformFlushFile(File);
    
    DWORD BytesWritten;
    BOOL err = WriteFile(File->Handle,
                         DataPtr,
                         DataSize,
                         &BytesWritten,
                         NULL);
    
    if (err == FALSE)
    {
        DisplayError(TEXT("WriteFile"));
        mprinte("Terminal failure: Unable to write to file.\n");
    }
}

void PlatformWriteBinaryToFile(file_t File, const char *Fmt, void *DataPtr, u32 DataLen)
{
    u32 ReqSize = 0;
    
    if (strcmp("c", Fmt) == 0)
    { // character
        ReqSize = DataLen * sizeof(char);
    }
    else if (strcmp("s", Fmt) == 0)
    { // c string
        ReqSize = DataLen * sizeof(char);
    }
    else if (strcmp("m", Fmt) == 0)
    { // mstring
        mstring *Data = (mstring*)DataPtr;
        for (u32 i = 0; i < DataLen; ++i)
        {
            ReqSize += sizeof(u32) + Data[i].Len * sizeof(char);
        }
    }
    else if (strcmp("i8", Fmt) == 0)
    { // i8
        ReqSize = DataLen * sizeof(i8);
    }
    else if (strcmp("i16", Fmt) == 0)
    { // i16
        ReqSize = DataLen * sizeof(i16);
    }
    else if (strcmp("i32", Fmt) == 0)
    { // i32
        ReqSize = DataLen * sizeof(i32);
    }
    else if (strcmp("i64", Fmt) == 0)
    { // i64
        ReqSize = DataLen * sizeof(i64);
    }
    else if (strcmp("u8", Fmt) == 0)
    {
        ReqSize = DataLen * sizeof(u8);
    }
    else if (strcmp("u16", Fmt) == 0)
    {
        ReqSize = DataLen * sizeof(u16);
    }
    else if (strcmp("u32", Fmt) == 0)
    {
        ReqSize = DataLen * sizeof(u32);
    }
    else if (strcmp("u64", Fmt) == 0)
    {
        ReqSize = DataLen * sizeof(u64);
    }
    else if (strcmp("r32", Fmt) == 0)
    {
        ReqSize = DataLen * sizeof(r32);
    }
    else if (strcmp("r64", Fmt) == 0)
    {
        ReqSize = DataLen * sizeof(r64);
    }
    else
    {
        mprinte("ERROR: Unknown binary print code!\n");
    }
    
    FlushBinaryWriteIfNeeded(File, ReqSize);
    
    if (strcmp("m", Fmt)  == 0)
    { // mstring, special print case
        mstring *Data = (mstring*)DataPtr;
        for (u32 i = 0; i < DataLen; ++i)
        {
            u32 Len = Data[i].Len;
            
            memcpy(File->Block.Brkp, &Len, sizeof(u32));
            File->Block.Brkp += sizeof(u32);
            
            memcpy(File->Block.Brkp, GetStr(&Data[i]), Data[i].Len * sizeof(char));
            File->Block.Brkp += Data[i].Len * sizeof(char);
        }
    }
    else if (ReqSize > 0)
    {
        // NOTE(Dustin): Will I ever need to worry about
        // reversing the byte order for standard filesystems?
        
        memcpy(File->Block.Brkp, DataPtr, ReqSize);
        File->Block.Brkp += ReqSize;
    }
}

void PlatformWriteToFile(file_t File, const char *Fmt, ...)
{
    va_list Args, Copy;
    va_start(Args, Fmt);
    
    // quick copy - need to determine if there is enough space in the buffer
    // for this write.
    va_copy(Copy, Args);
    u32 NeededChars = 1 + vsnprintf(NULL, 0, Fmt, Copy);
    va_end(Copy);
    
    char *Mem = File->Block.Brkp;
    File->Block.Brkp += NeededChars - 1;
    if (!Mem)
    {
        PlatformFlushFile(File);
        Mem = halloc<char>(&File->Block, NeededChars);
        
        if (!Mem)
        {
            mprinte("Platform write is larger than 2MB. Consider another approach!\n");
            
            va_end(Args);
            return;
        }
    }
    
    NeededChars = vsnprintf(Mem, NeededChars, Fmt, Args);
    
    va_end(Args);
}


void PlatformCopyFileIfChanged(const char *Destination, const char *Source)
{
    mstring DestinationFull = Win32NormalizePath(Destination);
    mstring SourceFull      = Win32NormalizePath(Source);
    
    BY_HANDLE_FILE_INFORMATION DestinationFileInfo;
    BY_HANDLE_FILE_INFORMATION SourceFileInfo;
    
    BOOL DstErr = GetFileAttributesEx(GetStr(&DestinationFull),
                                      GetFileExInfoStandard,
                                      &DestinationFileInfo);
    
    BOOL SrcErr = GetFileAttributesEx(GetStr(&SourceFull),
                                      GetFileExInfoStandard,
                                      &SourceFileInfo);
    
    if (!SrcErr)
    {
        DisplayError(TEXT("Could not find source file for copy!"));
    }
    else if (!DstErr)
    {
        BOOL Err = CopyFile(GetStr(&SourceFull),
                            GetStr(&DestinationFull),
                            NULL);
        
        if (!Err) DisplayError(TEXT("CopyFile"));
    }
    else
    {
        FILETIME DestinationLastWrite = DestinationFileInfo.ftLastWriteTime;
        FILETIME SourceLastWrite      = SourceFileInfo.ftLastWriteTime;
        
        if (CompareFileTime(&DestinationLastWrite, &SourceLastWrite) != 0)
        {
            BOOL Err = CopyFile(GetStr(&SourceFull),
                                GetStr(&DestinationFull),
                                NULL);
            
            if (!Err) DisplayError(TEXT("CopyFile"));
        }
    }
    
    MstringFree(&DestinationFull);
    MstringFree(&SourceFull);
}


//~ Logging

typedef struct
{
    HANDLE handle; // Stream handle (STD_OUTPUT_HANDLE or STD_ERROR_HANDLE).
    bool is_redirected; // True if redirected to file.
    bool is_wide; // True if appending to a UTF-16 file.
    bool is_little_endian; // True if file is UTF-16 little endian.
} Win32StandardStream;

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
file_internal Win32StandardStream Win32GetStandardStream(DWORD stream_type)
{
    Win32StandardStream result = {0};
    
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
                LARGE_INTEGER large = {0};
                u16 bom = 0;
                SetFilePointerEx(result.handle, large, 0, FILE_BEGIN);
                ReadFile(result.handle, &bom, 2, &dummy, 0);
                SetFilePointerEx(result.handle, large, 0, FILE_END);
                result.is_wide = (bom == (u16)0xfeff || bom == (u16)0xfffe);
                result.is_little_endian = (bom == (u16)0xfffe);
            }
        }
    }
    return result;
}

// Translates foreground/background color into a WORD text attribute.
file_internal WORD Win32TranslateConsoleColors(console_color text_color, console_color background_color)
{
    WORD result = 0;
    switch (text_color)
    {
        case ConsoleColor_White:
        result |=  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkGrey:
        result |= FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_Grey:
        result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
        case ConsoleColor_DarkRed:
        result |= FOREGROUND_RED;
        break;
        case ConsoleColor_Red:
        result |= FOREGROUND_RED | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkGreen:
        result |= FOREGROUND_GREEN;
        break;
        case ConsoleColor_Green:
        result |= FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkBlue:
        result |= FOREGROUND_BLUE;
        break;
        case ConsoleColor_Blue:
        result |= FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkCyan:
        result |= FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
        case ConsoleColor_Cyan:
        result |= FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkPurple:
        result |= FOREGROUND_RED | FOREGROUND_BLUE;
        break;
        case ConsoleColor_Purple:
        result |= FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkYellow:
        result |= FOREGROUND_RED | FOREGROUND_GREEN;
        break;
        case ConsoleColor_Yellow:
        result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
        default:
        break;
    }
    
    switch (background_color)
    {
        case ConsoleColor_White:
        result |=  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkGrey:
        result |=  FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_Grey:
        result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
        case ConsoleColor_DarkRed:
        result |= FOREGROUND_RED;
        break;
        case ConsoleColor_Red:
        result |= FOREGROUND_RED | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkGreen:
        result |= FOREGROUND_GREEN;
        break;
        case ConsoleColor_Green:
        result |= FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkBlue:
        result |= FOREGROUND_BLUE;
        break;
        case ConsoleColor_Blue:
        result |= FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkCyan:
        result |= FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
        case ConsoleColor_Cyan:
        result |= FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkPurple:
        result |= FOREGROUND_RED | FOREGROUND_BLUE;
        break;
        case ConsoleColor_Purple:
        result |= FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkYellow:
        result |= FOREGROUND_RED | FOREGROUND_GREEN;
        break;
        case ConsoleColor_Yellow:
        result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
        default:
        break;
    }
    
    return result;
}

// Prints a message to a platform stream. If the stream is a console, uses
// supplied colors.
file_internal void Win32PrintToStream(const char* message, Win32StandardStream stream, console_color text_color, console_color background_color)
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
}

void __Win32PrintMessage(console_color text_color, console_color background_color, char *fmt, va_list args)
{
    char *message = NULL;
    int chars_read = 1 + __Win32FormatString(message, 1, fmt, args);
    message = (char*)PlatformLocalAlloc(chars_read);
    __Win32FormatString(message, chars_read, fmt, args);
    
    // If we are in the debugger, output there.
    if (IsDebuggerPresent())
    {
        OutputDebugStringA(message);
    }
    else
    {
        // Otherwise, output to stdout.
        Win32StandardStream stream = Win32GetStandardStream(STD_OUTPUT_HANDLE);
        Win32PrintToStream(message, stream, text_color, background_color);
    }
}

void __Win32PrintError(console_color text_color, console_color background_color, char *fmt, va_list args)
{
    char *message = NULL;
    int chars_read = 1 + __Win32FormatString(message, 1, fmt, args);
    message = (char*)PlatformLocalAlloc(chars_read);
    __Win32FormatString(message, chars_read, fmt, args);
    
    if (IsDebuggerPresent())
    {
        OutputDebugStringA(message);
    }
    else
    {
        // Otherwise, output to stderr.
        Win32StandardStream stream = Win32GetStandardStream(STD_ERROR_HANDLE);
        Win32PrintToStream(message, stream, text_color, background_color);
    }
}

i32 PlatformFormatString(char *buff, i32 len, char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    int chars_read = __Win32FormatString(buff, len, fmt, list);
    va_end(list);
    
    return chars_read;
}

void PlatformPrintMessage(console_color text_color, console_color background_color, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    __Win32PrintMessage(text_color, background_color, fmt, args);
    va_end(args);
}

void PlatformPrintError(console_color text_color, console_color background_color, char* fmt, ...)
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
    __Win32PrintMessage(ConsoleColor_White, ConsoleColor_DarkGrey, fmt, args);
    va_end(args);
}

inline void mprinte(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    __Win32PrintError(ConsoleColor_Red, ConsoleColor_DarkGrey, fmt, args);
    va_end(args);
}

//~ Bit shifting

u32 PlatformCtzl(u64 Value)
{
    unsigned long TrailingZero = 0;
    
    if (_BitScanForward64(&TrailingZero, Value))
        return TrailingZero;
    else
        return 32;
}

u32 PlatformClzl(u64 Value)
{
    unsigned long LeadingZero = 0;
    
    if (_BitScanReverse64(&LeadingZero, Value))
        return 31 - LeadingZero;
    else
        return 32;
}

void* PlatformRequestMemory(u64 Size)
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
                           PAGE_READWRITE);          // Protection = no access
    
    return lpvBase;
}

void PlatformReleaseMemory(void *Ptr, u64 Size)
{
    BOOL bSuccess = VirtualFree(Ptr,           // Base address of block
                                0,             // Bytes of committed pages
                                MEM_RELEASE);  // Decommit the pages
    assert(bSuccess && "Unable to free a VirtualAlloc allocation!");
}

u64 PlatformGetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return (Result.QuadPart);
    
}

r32 PlatformGetSecondsElapsed(u64 start, u64 end)
{
    r32 Result = ((r32)(end - start) /
                  (r32)GlobalPerfCountFrequency);
    return(Result);
}

void PlatformGetClientWindowDimensions(u32 *Width, u32 *Height)
{
    RECT rect;
    GetClientRect(ClientWindow, &rect);
    
    *Width  = rect.right - rect.left;
    *Height = rect.bottom - rect.top;
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
    //~ Set the defaults of open files
    for (u32 i = 0; i < MAX_OPEN_FILES; ++i)
        OpenFiles[i].Handle = INVALID_HANDLE_VALUE;
    
    //~ Timing information
    // Setup timing information
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    
    UINT desired_scheduler_ms = 1;
    bool SleepIsGranular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
    
    char AppName[] = "Maple Engine";
    const char CLASS_NAME[] = "Maple Window Class";
    
    WNDCLASS wc = {0};
    wc.style = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);
    
    u32 ClientWindowWidth  = 1080;
    u32 ClientWindowHeight = 720;
    
    ClientWindow = CreateWindowEx(0,
                                  CLASS_NAME,
                                  AppName,
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
    
    //~ Memory Setup
    u64 MemorySize = _MB(50);
    GlobalMemoryPtr = PlatformRequestMemory(MemorySize);
    
    // Initialize memory manager
    FreeListAllocatorInit(&PermanantMemory, MemorySize, GlobalMemoryPtr);
    
    // Tagged heap. 3 Stages + 1 Resource/Asset = 4 Tags / frame
    // Keep 3 Blocks for each Tag, allowing for 12 Blocks overall
    TaggedHeapInit(&TaggedHeap, &PermanantMemory, _MB(24), _2MB, 10);
    PlatformHeap = TaggedHeapRequestAllocation(&TaggedHeap, PlatformTag);
    
    u64 StringArenaSize  = _MB(1);
    void *StringArenaPtr = FreeListAllocatorAlloc(&PermanantMemory, StringArenaSize);
    StringArenaInit(StringArenaPtr, StringArenaSize);
    
    //~ Setup Graphics
    ResourceRegistryInit(&ResourceRegistry, &PermanantMemory, 100);
    
    platform_window pWindow = {ClientWindow};
    
    RendererInit(&Renderer,
                 &PermanantMemory,
                 &ResourceRegistry,
                 &pWindow,
                 ClientWindowWidth,
                 ClientWindowHeight,
                 60);
    
    AssetRegistryInit(&AssetRegistry, Renderer, &PermanantMemory, 100);
    
    //~ Load some assets
    {
        tag_id_t Tag = {0, TAG_ID_ASSET, 0};
        tagged_heap_block HeapBlock = TaggedHeapRequestAllocation(&TaggedHeap, Tag);
        ConvertGltfMesh(&HeapBlock, "data/glTF/Fox/glTF/Fox.gltf");
        
        TaggedHeapReleaseAllocation(&TaggedHeap, Tag);
    }
    
    {
        simple_vertex Vertices[] = {
            // positions          // colors           // texture coords
            { {  0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.5f, 1.0f } },   // top right
            { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },   // bottom right
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },   // bottom left
        };
        
        u32 SimpleStride = sizeof(simple_vertex);
        
        simple_model_create_info CreateInfo = {};;
        CreateInfo.ResourceRegistry = &ResourceRegistry;
        CreateInfo.Vertices               = Vertices;
        CreateInfo.VerticesCount          = 3;
        CreateInfo.VertexStride           = sizeof(simple_vertex);
        CreateInfo.Indices                = NULL;
        CreateInfo.IndicesCount           = 0;
        CreateInfo.IndicesStride          = 0;
        CreateInfo.VertexShader           = "data/shaders/simple_vert.cso";
        CreateInfo.PixelShader            = "data/shaders/simple_frag.cso";
        CreateInfo.DiffuseTextureFilename = "W:/maple_engine/build/data/textures/wall.jpg";
        
        CreateAsset(&AssetRegistry, Asset_SimpleModel, &CreateInfo);
    }
    
    //~ Initialize ImGui stuff
    ImGuiContext *ctx = ImGui::CreateContext();
    if (!ImGui_ImplWin32_Init(ClientWindow))
        mprinte("Failed to initialzie ImGui!\n");
    
    resource_id DeviceId = Renderer->Device;
    
    ID3D11Device* Device = ResourceRegistry.Resources[DeviceId.Index]->Device.Handle;
    ID3D11DeviceContext* DeviceContext = ResourceRegistry.Resources[DeviceId.Index]->Device.Context;
    MapleDevGuiInit(Device, DeviceContext);
    
    //~ Load game code
    game_code GameCode = {};
    
    mstring GameDllCopy = Win32NormalizePath("example.dll");
    Win32LoadGameCode(&GameCode, GetStr(&GameDllCopy));
    
    //~ App Loop
    ShowWindow(ClientWindow, nCmdShow);
    
    u64 LastFrameTime = PlatformGetWallClock();
    r32 RefreshRate = 60.0f;
    r32 TargetSecondsPerFrame = 1 / RefreshRate;
    
    ClientIsRunning = true;
    MSG msg = {0};
    while (ClientIsRunning)
    {
        // Reload Dll if necessary
        FILETIME DllWriteTime = Win32GetLastWriteTime(GetStr(&GameDllCopy));
        if (CompareFileTime(&DllWriteTime, &GameCode.DllLastWriteTime) != 0)
        {
            Win32UnloadGameCode(&GameCode);
            Win32LoadGameCode(&GameCode, GetStr(&GameDllCopy));
        }
        
        // Message loop
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        u64 ClockNow = PlatformGetWallClock();
        r32 SecondsElapsedUpdate = PlatformGetSecondsElapsed(LastFrameTime, ClockNow);
        
        r32 SecondsElapsedPerFrame = SecondsElapsedUpdate;
        if (SecondsElapsedPerFrame < TargetSecondsPerFrame)
        {
            if (SleepIsGranular)
            {
                DWORD SleepMs = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedPerFrame));
                if (SleepMs > 0)
                {
                    Sleep(SleepMs);
                }
                
                SecondsElapsedPerFrame = PlatformGetSecondsElapsed(LastFrameTime, PlatformGetWallClock());
            }
        }
        else
        {
            // LOG: Missed frame rate!
        }
        
        LastFrameTime = PlatformGetWallClock();
        
        frame_params FrameParams = {};
        FrameParamsInit(&FrameParams, FrameCount++, LastFrameTime, &TaggedHeap, Renderer,
                        &ResourceRegistry, &AssetRegistry);
        
        GameCode.GameStageEntry(&FrameParams);
        
        FrameParams.GameStageEndTime     = PlatformGetWallClock();
        FrameParams.RenderStageStartTime = FrameParams.GameStageEndTime;
        
        RendererEntry(Renderer, &FrameParams);
        
        FrameParamsFree(&FrameParams);
    }
    
    MstringFree(&GameDllCopy);
    
    //~ Close down ImGui
    MapleDevGuiFree();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    //~ Free Graphics layer
    RendererShutdown(&Renderer, &PermanantMemory);
    AssetRegistryFree(&AssetRegistry, &PermanantMemory);
    ResourceRegistryFree(&ResourceRegistry, &PermanantMemory);
    
    //~ Close up the memory pools
    StringArenaFree();
    FreeListAllocatorAllocFree(&PermanantMemory, StringArenaPtr);
    
    // Free up the tagged heap for per-frame info
    TaggedHeapFree(&TaggedHeap, &PermanantMemory);
    
    // free up the global memory
    FreeListAllocatorFree(&PermanantMemory);
    PlatformReleaseMemory(GlobalMemoryPtr, MemorySize);
    
    return (0);
}

file_internal void SetFullscreen(bool fullscreen)
{
    if (GlobalIsFullscreen != fullscreen)
    {
        GlobalIsFullscreen = fullscreen;
        
        if (GlobalIsFullscreen)
        {
            GetWindowRect(ClientWindow, &ClientWindowRect);
            
            UINT window_style = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION |
                                                        WS_SYSMENU |
                                                        WS_THICKFRAME |
                                                        WS_MINIMIZEBOX |
                                                        WS_MAXIMIZEBOX);
            SetWindowLong(ClientWindow, GWL_STYLE, window_style);
            
            HMONITOR hmonitor = MonitorFromWindow(ClientWindow, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitor_info = {};
            GetMonitorInfo(hmonitor, (LPMONITORINFO)&monitor_info);
            
            SetWindowPos(ClientWindow, HWND_TOP,
                         monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                         SWP_FRAMECHANGED | SWP_NOACTIVATE);
            
            ShowWindow(ClientWindow, SW_MAXIMIZE);
        }
        else
        {
            SetWindowLong(ClientWindow, GWL_STYLE, WS_OVERLAPPEDWINDOW);
            
            SetWindowPos(ClientWindow, HWND_NOTOPMOST,
                         ClientWindowRectOld.left,
                         ClientWindowRectOld.top,
                         ClientWindowRectOld.right - ClientWindowRectOld.left,
                         ClientWindowRectOld.bottom - ClientWindowRectOld.top,
                         SWP_FRAMECHANGED | SWP_NOACTIVATE);
            
            ShowWindow(ClientWindow, SW_NORMAL);
        }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!ClientIsRunning) return DefWindowProc(hwnd, uMsg, wParam, lParam);
    
    // Update ImGui
    ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);
    ImGuiIO& io = ImGui::GetIO();
    
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
        
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            if (!io.WantCaptureKeyboard)
            {
                bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                switch (wParam)
                {
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
                                // NOTE(Dustin): Disabling fullscreen fails to restore the
                                // previous window's position and defaults to the top left
                                // corner
                                if (!GlobalIsFullscreen)
                                    GetClientRect(hwnd, &ClientWindowRectOld);
                                
                                SetFullscreen(!GlobalIsFullscreen);
                            } break;
                        } break;
                    } break;
                }
            }
        } break;
        
        case WM_SIZE:
        {
            //ClientWindowRectOld = ClientWindowRect;
            GetClientRect(hwnd, &ClientWindowRect);
            
            int Width  = ClientWindowRect.right - ClientWindowRect.left;
            int Height = ClientWindowRect.bottom - ClientWindowRect.top;
            
            RendererResize(Renderer, &ResourceRegistry);
        } break;
        
        default: break;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}