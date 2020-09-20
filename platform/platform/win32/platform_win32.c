
// TODO(Dustin):
// Build script - change name of pdb file so that hot reloading works with VS debugger
// Loop editor, useful for fine tuning gameplay mechanics, HH Day 23 is a good walkthrough
// Profiling Tools
// Debug Tools
// Audio
// And about a million other things...

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 512
#endif

// Library function declarations
#include "library_loader.c"

typedef struct platform_window
{
    HWND Window;
} platform_window;

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
file_global bool NeedsToResize   = false;

file_global input GlobalPerFrameInput;

// Frame Info
file_global u64 FrameCount = 0;

typedef struct library_code
{
    HMODULE           GraphicsHandle;
    FILETIME          GraphicsDllLastWriteTime;
    
    HMODULE           GameHandle;
    FILETIME          GameDllLastWriteTime;
    
} library_code;

file_global library_code LibraryCode;

platform    *PlatformApi;

file_internal void SetFullscreen(bool fullscreen);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

mstr Win32NormalizePath(char* path);
mstr Win32GetExeFilepath();

i32  __Win32FormatString(char *buff, i32 len, char *fmt, va_list list);
void __Win32PrintMessage(console_color text_color, console_color background_color, char *fmt, va_list args);
void __Win32PrintError(console_color text_color, console_color background_color, char *fmt, va_list args);

file_internal void MapleShutdown();

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
    void* Result = NULL;
    
#if 0
    if (!PlatformHeap.Start)
        PlatformHeap = TaggedHeapRequestAllocation(&Core->TaggedHeap, PlatformTag);
    
    Result = TaggedHeapBlockAlloc(&PlatformHeap, Size);
    if (!Result)
    {
        PlatformHeap.Brkp = PlatformHeap.Start;
        
        Result = TaggedHeapBlockAlloc(&PlatformHeap, Size);
        if (!Result)
            mprinte("Error allocating from Platform Heap Allocator!\n");
    }
#else
    
    // TODO(Dustin): Temporary fix for not having a tagged heap/string right now
    Result = malloc(Size);
    
#endif
    
    return Result;
}

void PlatformFatalError(char *Fmt, ...)
{
    va_list Args;
    va_start(Args, Fmt);
    
    char *Message = NULL;
    int CharsRead = 1 + __Win32FormatString(Message, 1, Fmt, Args);
    Message = (char*)PlatformLocalAlloc(CharsRead);
    __Win32FormatString(Message, CharsRead, Fmt, Args);
    
    MessageBox(ClientWindow, Message, "FATAL ERROR", MB_OK);
    MapleShutdown();
    exit(1);
}

//~ Game Code Hot loading

file_internal FILETIME Win32GetLastWriteTime(const char *Filename)
{
    FILETIME LastWriteTime = {};
    
    BY_HANDLE_FILE_INFORMATION FileInfo;
    
    BOOL Err = GetFileAttributesEx(Filename,
                                   GetFileExInfoStandard,
                                   &FileInfo);
    
    if (Err) LastWriteTime = FileInfo.ftLastWriteTime;
    
    return LastWriteTime;
}


file_internal void Win32LoadGraphicsCode(const char *GraphicsDllName)
{
    // TODO(Dustin): Last DLL write time
    
    // hard code the name...for now
    const char *Name = "maple_vk.dll";
    
    HMODULE GraphicsDll = LoadLibrary(Name);
    
    if (GraphicsDll)
    {
    }
    else
    {
        PlatformFatalError("Could not load the graphics dll!");
    }
    
#define GRAPHICS_EXPORTED_FUNCTION(fun)                                     \
    if (!(Graphics.fun = (PFN_##fun)GetProcAddress(GraphicsDll, #fun))) {            \
                          PlatformFatalError("Could not load exported function: %s\n", #fun); \
}

#include "../../../graphics/graphics_functions.inl"

LibraryCode.GraphicsHandle = GraphicsDll;
}

file_internal void Win32LoadGameCode(const char *GameDllName)
{
    // TODO(Dustin): Last DLL write time
    mstr GameDllCopy = Win32NormalizePath("game_temp.dll");
    
    LibraryCode.GameDllLastWriteTime = Win32GetLastWriteTime(GameDllName);
    
    CopyFile(GameDllName, mstr_to_cstr(&GameDllCopy), FALSE);
    
    HMODULE VoxelDll = LoadLibrary(mstr_to_cstr(&GameDllCopy));
    
    if (VoxelDll)
    {
    }
    else
    {
        PlatformFatalError("Could not load the graphics dll!");
    }
    
#define VOXEL_EXPORTED_FUNCTION(fun)                                     \
    if (!(Game.fun = (PFN_##fun)GetProcAddress(VoxelDll, #fun))) {            \
                      PlatformFatalError("Could not load exported function: %s\n", #fun); \
}

#include "../../../game/voxel_pfn.inl"

LibraryCode.GameHandle = VoxelDll;
}


void Win32UnloadGameCode()
{
    if (LibraryCode.GameHandle)
        FreeLibrary(LibraryCode.GameHandle);
    LibraryCode.GameHandle = 0;
    
#define VOXEL_EXPORTED_FUNCTION(fun) Game.fun = NULL;
#include "../../../game/voxel_pfn.inl"
    
}


//~ File I/O

// Takes a path relative to the executable, and normalizes it into a full path.
// Tries to handle malformed input, but returns an empty string if unsuccessful.
mstr Win32NormalizePath(char* path)
{
    // If the string is null or has length < 2, just return an empty one.
    if (!path || !path[0] || !path[1]) return mstr_init(0, 0);
    
    // Start with our relative path appended to the full executable path.
    mstr exe_path = Win32GetExeFilepath();
    mstr result = cstr_add(mstr_to_cstr(&exe_path), exe_path.Len, path, strlen(path));
    
    char *Str = mstr_to_cstr(&result);
    
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
                if (!has_separator) return mstr_init(0, 0);
            }
        }
        if (i > 0 && Str[i - 1] == '/') last_separator = i - 1;
    }
    
    mstr_free(&exe_path);
    return result;
}

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
mstr Win32GetExeFilepath()
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
    
    int len = (pos - exe) - 1; // Remove the final '/'
    mstr result = mstr_init(exe, len);
    
    return result;
}

#if 0

file_t PlatformFindAvailableFileHandle()
{
    file_t Result = NULL;
    
    // Negate it so that 1s are unused bits
    // and 0s are used bits
    u32 NegatedBitfield = ~OpenFileBitField;
    u32 Index = PlatformCtz(NegatedBitfield);
    
    if (Index < 32)
        
    {
        Result = OpenFiles + Index;
    }
    else
    {
        mprinte("No available file handles!");
    }
    
    BIT_TOGGLE_1(OpenFileBitField, Index);
    
    return Result;
}


file_t PlatformLoadFile(char *Filename, bool Append)
{
    file_t Result = NULL;
    
    // Find an open file handle
    Result = PlatformFindAvailableFileHandle();
    
    if (Result)
    {
        mstr AbsPath = Win32NormalizePath(Filename);
        
#if 0
        Result->Handle = CreateFileA(mstr_to_cstr(&AbsPath),
                                     GENERIC_READ,
                                     0,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL);
        
        if (Result->Handle == INVALID_HANDLE_VALUE)
        {
            DisplayError(TEXT("CreateFile"));
            mstr_free(&AbsPath);
            
            BIT_TOGGLE_0(OpenFileBitField, Result->Index);
            Result = NULL;
            
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
            
            BIT_TOGGLE_0(OpenFileBitField, Result->Index);
            Result = NULL;
            
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
            mprinte("File \"%s\" is too large and should probably be streamed from disc!\n", 
                    mstr_to_cstr(&AbsPath));
            CloseHandle(Result->Handle);
            Result->Handle = INVALID_HANDLE_VALUE;
            
            BIT_TOGGLE_0(OpenFileBitField, Result->Index);
            Result = NULL;
            
            return Result;
        }
        
        // Reads the file into memory 
#if 0
        DWORD bytes_read = 0;
        Result->FileSize = file_size;
        Result->Memory = PlatformRequestMemory(file_size);
        
        err = ReadFile(Result->Handle,
                       Result->Memory,
                       low_size,
                       &bytes_read,
                       NULL);
        
        if (err == 0)
        {
            mprinte("Unable to read file!\n");
            Result = File_UnableToRead;
        }
#endif // cutofff for reading file directly into memory
        
#else // read the file with clib
        
        if( fopen_s( &Result->cFile, mstr_to_cstr(&AbsPath), "r+" ) != 0 )
        {
            mprinte( "The file fscanf.out was not opened: %s\n", mstr_to_cstr(&AbsPath));
        }
        else
        {
        }
#endif // cutoff for the CreateFileA -> winapi
        
    }
    else
    {
        mprinte("Unable to find an open file handle. Please close a file handle to open another!\n");
    }
    
    Result->NextWriteOffset = 0;
    Result->Offset = 0;
    
    return Result;
}


file_t PlatformOpenFile(char *Filename, bool Append)
{
    file_t Result = NULL;
    
    // Find an open file handle
    Result = PlatformFindAvailableFileHandle();
    
    if (Result)
    {
        mstr AbsPath = Win32NormalizePath(Filename);
        
        if (Append)
        {
            Result->Handle = CreateFileA(mstr_to_cstr(&AbsPath), 
                                         FILE_APPEND_DATA, 
                                         FILE_SHARE_READ, 
                                         NULL, // No security
                                         OPEN_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
        }
        else
        {
            Result->Handle = CreateFileA(mstr_to_cstr(&AbsPath), 
                                         GENERIC_WRITE, 
                                         0, 
                                         0,
                                         TRUNCATE_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
        }
        
        // file doesn't currently exist
        if (Result->Handle == INVALID_HANDLE_VALUE)
        {
            Result->Handle = CreateFileA(mstr_to_cstr(&AbsPath), GENERIC_WRITE, 0, 0, CREATE_NEW,
                                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
        }
        
        if (Result->Handle == INVALID_HANDLE_VALUE)
        {
            DisplayError(TEXT("CreateFile"));
            _tprintf(TEXT("Terminal failure: Unable to open file \"%s\" for write.\n"), 
                     mstr_to_cstr(&AbsPath));
            
            BIT_TOGGLE_0(OpenFileBitField, Result->Index);
            Result = NULL;
        }
        
        // TODO(Dustin): Acquire some memory to write to!
    }
    else
    {
        mprinte("Unable to find an open file handle. Please close a file handle to open another!\n");
    }
    
    Result->NextWriteOffset = 0;
    Result->Offset = 0;
    
    return Result;
}

void PlatformFlushFile(file_t File)
{
    DWORD SeekError = GetLastError();
    
    void *MemPtr = File->Memory;
    u64 FileSize = File->Offset;
    
    // TODO(Dustin): Set the FilePointer to be at each offset
    // and use FILE_CURRENT instead of FILE_BEGIN
    DWORD SeekResult = SetFilePointer(File->Handle,
                                      File->NextWriteOffset,
                                      NULL,
                                      FILE_BEGIN);
    
    SeekError = GetLastError();
    if (SeekResult == INVALID_SET_FILE_POINTER && SeekError == INVALID_SET_FILE_POINTER)
    {
        DisplayError(TEXT("SetFilePointer"));
        mprinte("Terminal failure: Unable to seek to the next file write position.\n");
        return;
    }
    
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
    else
    {
        File->NextWriteOffset += BytesWritten;
        File->Offset = 0;
    }
}

void PlatformWriteFile(file_t File, char *Fmt, ...)
{
    if (!File->Memory)
    {
        File->Memory = PlatformRequestMemory(FILE_WRITE_SIZE);
        File->Offset = 0;
    }
    
    va_list Args, Copy;
    va_start(Args, Fmt);
    
    // quick copy - need to determine if there is enough space in the buffer
    // for this write.
    va_copy(Copy, Args);
    i32 MaybeCountWritten = 1 + vsnprintf(NULL, 0, Fmt, Copy);
    va_end(Copy);
    
    if (MaybeCountWritten >= FILE_WRITE_SIZE - File->Offset |
        FILE_WRITE_SIZE <= File->Offset)
    {
        PlatformFlushFile(File);
    }
    
    
    MaybeCountWritten = vsnprintf((char*)File->Memory + File->Offset,
                                  MaybeCountWritten,
                                  Fmt,
                                  Args);
    
    File->Offset += MaybeCountWritten;
}

void PlatformCloseFile(file_t File)
{
    if (File->Memory) PlatformReleaseMemory(File->Memory, File->FileSize);
    File->Memory = NULL;
    
    if (File->Handle) CloseHandle(File->Handle);
    File->FileSize = 0;
    File->Handle   = INVALID_HANDLE_VALUE;
    
    BIT_TOGGLE_0(OpenFileBitField, File->Index);
}

void* PlatformGetFileBuffer(file_t File)
{
    return File->Memory;
}

// Get the current size of the file. Only useful when reading files.
u64 PlatformGetFileSize(file_t File)
{
    return File->FileSize;
}

void PlatformReadFile(file_t File, char *Fmt, ...)
{
#if 0
    if (File->Memory)
    {
        va_list Args;
        va_start(Args, Fmt);
        
        // TODO(Dustin): Take a look at return values to make sure the 
        // return was correct.
        int Ret = vsscanf(File->Memory + File->Offset, Fmt, Args);
        
        // TODO(Dustin): Check for EOF?
        if (File->Offset + Ret > File->FileSize)
        {
            mprinte("Read file read past the data stream!\n");
        }
        
        va_end(Args);
    }
#else
    
    va_list Args;
    va_start(Args, Fmt);
    
    vfscanf(File->cFile, Fmt, Args);
    
    va_end(Args);
    
#endif
}

typedef enum seek_type
{
    SeekType_Start,
    SeekType_End,
    SeekType_Current,
} seek_type;

void PlatformFileSeek(file_t File, u64 Offset, seek_type SeekStart)
{
    u64 RealOffset = 0;
    if (SeekStart == SeekType_Start)
    {
        RealOffset = Offset;
    }
    else if (SeekStart == SeekType_End)
    {
        RealOffset = File->FileSize + Offset;
    }
    else if (SeekStart == SeekType_Current)
    {
        RealOffset = File->Offset + Offset;
    }
}


// "Save" the current offset into a file. This is particularly useful
// if a user is scanning over and plan to rewind their position. Currently,
// only 5 savepoints are allowed. The Savepoints structure acts as a stack. 
bool PlatformFileSetSavepoint(file_t File)
{// TODO(Dustin): 
    bool Result = false;
    return Result;
}

// Restore the save point at the top of the stack.
bool PlatformFileRestoreSavepoint(file_t File)
{// TODO(Dustin): 
    bool Result = false;
    return Result;
}

void PlatformReadFile(file_t File, void *Result, u32 Len, const char *Fmt)
{// TODO(Dustin): 
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

#endif

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
    {
        if (freopen_s(&fp, "CONIN$", "r", stdin) != 0)
            result = false;
    }
    else
    {
        setvbuf(stdin, NULL, _IONBF, 0);
    }
    
    // Redirect STDOUT if the console has an output handle
    if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0)
            result = false;
    }
    else
    {
        setvbuf(stdout, NULL, _IONBF, 0);
    }
    
    // Redirect STDERR if the console has an error handle
    if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&fp, "CONOUT$", "w", stderr) != 0)
            result = false;
    }
    else
    {
        setvbuf(stderr, NULL, _IONBF, 0);
    }
    
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

u32 PlatformClz(u32 Value)
{
    unsigned long LeadingZero = 0;
    
    if (_BitScanReverse64(&LeadingZero, Value))
        return 31 - LeadingZero;
    else
        return 32;
}

u32 PlatformCtz(u32 Value)
{
    unsigned long TrailingZero = 0;
    
    if (Value == 0) return 0;
    else if (_BitScanForward64(&TrailingZero, Value))
        return TrailingZero;
    else
        return 32;
}

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

window_rect PlatformGetClientWindowRect()
{
    window_rect Result = {0};
    
    RECT Rect;
    GetClientRect(ClientWindow, &Rect);
    
    Result.Left   = Rect.left;
    Result.Right  = Rect.right;
    Result.Top    = Rect.top;
    Result.Bottom = Rect.bottom;
    
    return Result;
}

void PlatformGetClientWindow(platform_window **Window)
{
    HWND **Win32Window = (HWND**)Window;
    *Win32Window = &ClientWindow;
}

file_internal void MapleShutdown()
{
    Game.voxel_shutdown();
    Graphics.shutdown_graphics();
    globals_free();
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
    //~ Set the defaults of open files
#if 0
    for (u32 i = 0; i < MAX_OPEN_FILES; ++i)
        OpenFiles[i].Handle = INVALID_HANDLE_VALUE;
#endif
    
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
    
    u32 ClientWindowWidth  = 1920;
    u32 ClientWindowHeight = 1080;
    
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
    
    //~ Initialize the globals3
    
    u32 RealWidth, RealHeight;
    PlatformGetClientWindowDimensions(&RealWidth, &RealHeight);
    
    assetsys_mount_point_create_info MountInfos[] = {
        { .Path = "data/shaders"             , .MountName = "shaders", .ParentMountName = "root" },
    };
    
    globals_create_info GlobalInfo = {0};
    GlobalInfo.Memory.Size                  = _MB(400);
    GlobalInfo.AssetSystem.ExecutablePath   = NULL;
    GlobalInfo.AssetSystem.MountPoints      = MountInfos;
    GlobalInfo.AssetSystem.MountPointsCount = sizeof(MountInfos)/sizeof(MountInfos[0]);
    globals_init(&GlobalInfo);
    
    file_print_directory_tree("root");
    mprint("\n\n");
    file_print_directory_tree("shaders");
    
    //file_id Fid = file_open("simple_tri.vert", true, "shaders", FileMode_Read);
    file_id NewFid = file_open("new_file_test.txt", true, "root", FileMode_Write);
    file_close(NewFid);
    
    void *Buffer = 0;
    u64 BufferSize = 0;
    
    file_load("simple_tri.vert", true, "shaders", Buffer, BufferSize);
    
    PlatformApi = (platform*)memory_alloc(Core->Memory, sizeof(platform));
    PlatformApi->Memory          = Core->Memory;
    PlatformApi->open_file       = &file_open;
    PlatformApi->load_file       = &file_load;
    PlatformApi->close_file      = &file_close;
    PlatformApi->file_get_size   = &file_get_size;
    PlatformApi->file_get_fsize  = &file_get_fsize;
    //PlatformApi->flush_file      = &PlatformFlushFile;
    //PlatformApi->write_file      = &PlatformWriteFile;
    //PlatformApi->get_file_buffer = &PlatformGetFileBuffer;
    PlatformApi->mprint          = &mprint;
    PlatformApi->mprinte         = &mprinte;
    PlatformApi->get_client_window_dimensions = &PlatformGetClientWindowDimensions;
    PlatformApi->get_client_window = &PlatformGetClientWindow;
    PlatformApi->request_memory = PlatformRequestMemory;
    PlatformApi->release_memory = PlatformReleaseMemory;
    
    //~ Load game code
    
    Win32LoadGraphicsCode("");
    
    const char *GameDllName = "maple_voxel.dll";
    Win32LoadGameCode(GameDllName);
    
    {
        graphics_create_info Info = {};
        Info.Window   = (window_t)&ClientWindow;
        Info.Platform = PlatformApi;
        Graphics.initialize_graphics(&Info);
    }
    
    vec3 DefaultPosition = {0, 40, -10};
    
    camera PlayerCamera;
    camera_default_init(&PlayerCamera, DefaultPosition);
    
    //~ Render Loop
    ShowWindow(ClientWindow, nCmdShow);
    
    u64 LastFrameTime = PlatformGetWallClock();
    r32 RefreshRate = 60.0f;
    r32 TargetSecondsPerFrame = 1 / RefreshRate;
    
    u32 LoadDllCounter = 0;
    
    ClientIsRunning = true;
    MSG msg = {0};
    while (ClientIsRunning)
    {
        GlobalPerFrameInput.KeyPress = 0;
        
        frame_params FrameParams = {0};
        FrameParams.Frame    = FrameCount;
        FrameParams.Graphics = &Graphics;
        FrameParams.Platform = PlatformApi;
        FrameParams.Camera   = &PlayerCamera;
        
        // NOTE(Dustin): When check the file time, if there is not a small delay in the check
        // then the DLL is loaded twice. In order to solve this, rather than checking every frame,
        // check N times per second.
        // Reload Dll if necessary
#if 1
        FILETIME DllWriteTime = Win32GetLastWriteTime(GameDllName);
        if (LoadDllCounter++ >= RefreshRate / 1)
        {
            LoadDllCounter = 0;
            
            if (CompareFileTime(&DllWriteTime, &LibraryCode.GameDllLastWriteTime) != 0)
            {
                Game.voxel_shutdown();
                Win32UnloadGameCode();
                Win32LoadGameCode(GameDllName);
            }
        }
#endif
        
        // Message loop
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        FrameParams.Input = GlobalPerFrameInput;
        
        FrameParams.GameStageEndTime     = PlatformGetWallClock();
        FrameParams.RenderStageStartTime = FrameParams.GameStageEndTime;
        
        Graphics.begin_frame();
        
        {
            // Issue some interesting frame commands :)
            //Game.voxel_entry(&FrameParams);
        }
        
        //Graphics.execute_command_list(PolygonalWorld.CommandList);
        
        end_frame_cmd EndFrame = {};
        Graphics.end_frame(&EndFrame);
        
        FrameParams.RenderStageEndTime = PlatformGetWallClock();
        
        FrameCount++;
        
        //#endif
        
#if 0
        // Log frame timings...
        local_persist r32 FrameAccumulator = 0.0f;
        
        r32 GameElapsed   = PlatformGetSecondsElapsed(FrameParams.FrameStartTime, FrameParams.GameStageEndTime);
        r32 RenderElapsed = PlatformGetSecondsElapsed(FrameParams.RenderStageStartTime, FrameParams.RenderStageEndTime);
        //r32 GpuElapsed    = PlatformGetSecondsElapsed(FrameParams.GpuStageStartTime, FrameParams.GpuStageEndTime);;
        
        u32 Fps = static_cast<u32>(1.0f / (GameElapsed + RenderElapsed));
        
        // Prints the ms for each stage in the frame
        PlatformPrintMessage(ConsoleColor_Red, ConsoleColor_DarkGrey, "Frame: %ld\n", FrameParams.Frame);
        PlatformPrintMessage(ConsoleColor_Green, ConsoleColor_DarkGrey, "\tGame Stage Time:  \t%f ms\n",
                             GameElapsed * 1000.0f);
        PlatformPrintMessage(ConsoleColor_Green, ConsoleColor_DarkGrey, "\tRender Stage Time:\t%f ms\n",
                             RenderElapsed * 1000.0f);
        //PlatformPrintMessage(EConsoleColor::Green, EConsoleColor::DarkGrey, "\tGpu Stage Time:   \t%f ms\n",
        //GpuElapsed * 1000.0f);
        PlatformPrintMessage(ConsoleColor_Green, ConsoleColor_DarkGrey, "\tFPS:              \t%d\n", Fps);
#endif
        
#if 0
        if (NeedsToResize)
            RendererResize();
#endif
        
        //~ Meet frame rate, if necessary
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
    }
    
    Graphics.wait_for_last_frame();
    MapleShutdown();
    
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
            MONITORINFOEX monitor_info = {0};
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
    //ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);
    //ImGuiIO& io = ImGui::GetIO();
    
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
            //if (!io.WantCaptureKeyboard)
            {
                bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                
                if (alt) GlobalPerFrameInput.KeyPress |= Key_Alt;
                
                switch (wParam)
                {
                    case 'W':      GlobalPerFrameInput.KeyPress |= Key_w;     break;
                    case 'S':      GlobalPerFrameInput.KeyPress |= Key_s;     break;
                    case 'A':      GlobalPerFrameInput.KeyPress |= Key_a;     break;
                    case 'D':      GlobalPerFrameInput.KeyPress |= Key_d;     break;
                    
                    case VK_UP:    GlobalPerFrameInput.KeyPress |= Key_Up;    break;
                    case VK_DOWN:  GlobalPerFrameInput.KeyPress |= Key_Down;  break;
                    case VK_LEFT:  GlobalPerFrameInput.KeyPress |= Key_Left;  break;
                    case VK_RIGHT: GlobalPerFrameInput.KeyPress |= Key_Right; break;
                    
                    case VK_F1:    GlobalPerFrameInput.KeyPress |= Key_F1;    break;
                    case VK_F2:    GlobalPerFrameInput.KeyPress |= Key_F2;    break;
                    case VK_F3:    GlobalPerFrameInput.KeyPress |= Key_F3;    break;
                    case VK_F4:    GlobalPerFrameInput.KeyPress |= Key_F4;    break;
                    case VK_F5:    GlobalPerFrameInput.KeyPress |= Key_F5;    break;
                    
                    case '0': GlobalPerFrameInput.KeyPress |= Key_0;          break;
                    case '1': Graphics.set_render_mode(RenderMode_Solid);     break;
                    case '2': Graphics.set_render_mode(RenderMode_Wireframe); break;
                    case '3': Graphics.set_render_mode(RenderMode_NormalVis); break;
                    
                    case VK_SPACE:
                    {
                        RenderDevGui = !RenderDevGui;
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
                                // NOTE(Dustin): Disabling fullscreen fails to restore the
                                // previous window's position and defaults to the top left
                                // corner
                                if (!GlobalIsFullscreen)
                                    GetClientRect(hwnd, &ClientWindowRectOld);
                                
                                SetFullscreen(!GlobalIsFullscreen);
                                NeedsToResize = true;
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
            
            NeedsToResize = true;
        } break;
        
        default: break;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}