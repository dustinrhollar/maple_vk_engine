#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <Tchar.h>
#include <strsafe.h>

// Add logger into win32
#include "logger.h"
#include "logger.cpp"

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

struct CodeDLL
{
    HMODULE game;
} GlobalCodeDLL;

ErrorCode SetError;
char ErrorBuffer[512];

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
        printf("FATAL ERROR: Unable to output error code.\n");
    }
    
    _tprintf(TEXT("ERROR: %s\n"), (LPCTSTR)lpDisplayBuf);
    
    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

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

DynamicArray<jstring> Win32FindAllFilesInDirectory(jstring &directory, jstring delimiter)
{
    DynamicArray<jstring> files = DynamicArray<jstring>();
    
    jstring path = directory + delimiter;
    
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
    
    return files;
}

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
jstring Win32GetExeFilepath()
{
    // Get the full filepath
    char exe[WIN32_STATE_FILE_NAME_COUNT];
    size_t filename_size = sizeof(exe);
    
    DWORD filepath_size = GetModuleFileNameA(0, exe, filename_size);
    
    //printf("Filepath %s\n", exe);
    
    // find last "\"
    char *pos = 0;
    for (char *Scanner = exe; *Scanner; ++Scanner)
    {
        if (*Scanner == '\\')
        {
            pos = Scanner + 1;
        }
    }
    
    int len = (pos - exe) + 1;
    jstring result = InitJString(exe, (pos - exe));
    
    return result;
}

struct Win32StandardStream
{
    HANDLE handle; // Stream handle (STD_OUTPUT_HANDLE or STD_ERROR_HANDLE).
    bool is_redirected; // True if redirected to file.
    bool is_wide; // True if appending to a UTF-16 file.
    bool is_little_endian; // True if file is UTF-16 little endian.
};

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

jstring Win32LoadFile(jstring &directory, jstring &filename)
{
    jstring fullpath = InitJString(directory.len + filename.len);
    AddJString(fullpath, directory, filename);
    
    HANDLE handle = CreateFileA(fullpath.GetCStr(),
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
        printf("File \"%s\" is too large and should probably be streamed from disc!\n", fullpath.GetCStr());
        CloseHandle(handle);
        
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
    
    return result;
}

void Win32WriteBufferToFile(jstring &file, void *buffer, u32 size)
{
    HANDLE handle = CreateFileA(file.GetCStr(), GENERIC_WRITE, 0, 0,
                                TRUNCATE_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
    
    if (handle == INVALID_HANDLE_VALUE)
    {
        DisplayError(TEXT("CreateFile"));
        
        handle = CreateFileA(file.GetCStr(), GENERIC_WRITE, 0, 0, CREATE_NEW,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
    }
    
    if (handle == INVALID_HANDLE_VALUE)
    {
        CloseHandle(handle);
        
        DisplayError(TEXT("CreateFile"));
        _tprintf(TEXT("Terminal failure: Unable to open file \"%s\" for write.\n"), file.GetCStr());
        return;
    }
    
    DWORD bytes_written = 0;
    BOOL err = WriteFile(handle,         // open file handle
                         buffer,         // start of data to write
                         size,           // number of bytes to write
                         &bytes_written, // number of bytes that were written
                         NULL);          // no overlapped structure
    
    if (err == FALSE)
    {
        DisplayError(TEXT("WriteFile"));
        printf("Terminal failure: Unable to write to file.\n");
    }
    else
    {
        if (bytes_written != size)
        {
            // This is an error because a synchronous write that results in
            // success (WriteFile returns TRUE) should write all data as
            // requested. This would not necessarily be the case for
            // asynchronous writes.
            printf("Error: dwBytesWritten != dwBytesToWrite\n");
        }
    }
    
    CloseHandle(handle);
}

void Win32DeleteFile(jstring &file)
{
    if (!DeleteFile(file.GetCStr()))
    {
        DisplayError(TEXT("DeleteFile"));
    }
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

// TODO(Dustin): Integrate Mattew's logger
void Win32EnableLogging()
{
    int hConHandle;
    long lStdHandle;
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    
    // allocate console for the app
    // TODO: Inorder to it to spawn in the parent process,
    // need to handle exit conditions
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
    {
        AllocConsole();
    }
    
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stderr);
    freopen("CONOUT$", "w", stdout);
    
    //Clear the error state for each of the C++ standard stream objects.
    std::wclog.clear();
    std::clog.clear();
    std::wcout.clear();
    std::cout.clear();
    std::wcerr.clear();
    std::cerr.clear();
    std::wcin.clear();
    std::cin.clear();
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
        printf("Error enumerating Instance extension properties!\n");
    }
    
    ep = palloc<VkExtensionProperties>();
    err = vk::vkEnumerateInstanceExtensionProperties(NULL, &count, ep);
    if (err)
    {
        printf("Unable to retrieve enumerated extension properties!\n");
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
    
    printf("Could not find win32 vulkan surface extension!\n");
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
    vk::ShutdownVulkan();
    ecs::ShutdownECS();
    mm::ShutdownMemoryManager();
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
    
    // Initialize the Logger
    // TODO(Dustin): Integrate Matthew's logger
    // TODO(Dustin): Flush logs to a file. when an app crashes,
    // need to have a way to maintain the log reports.
    InitializeLogger();
    
    
    // Initialize Management Routines
    // NOTE(Dustin): Macros ar defined in engine_config.h
    mm::InitializeMemoryManager(MEMORY_USAGE, TRANSIENT_MEMORY);
    ecs::InitializeECS();
    
    // Enable graphics
    if (!vk::InitializeVulkan())
    {
        printf("Unable to initialize Vulkan!\n");
        //glfwTerminate();
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
    
    ClientIsRunning = true;
    MSG msg = {};
    while (ClientIsRunning)
    {
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
        FrameInput input = {};
        GameUpdateAndRender(input);
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
    switch (uMsg)
    {
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
        
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            switch (wParam)
            {
                case 'V':
                {
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
        
        case WM_SYSCHAR: break;
        
        case WM_SIZE:
        {
            ClientWindowRectOld = ClientWindowRect;
            GetClientRect(hwnd, &ClientWindowRect);
            
            int width = ClientWindowRect.right - ClientWindowRect.left;
            int height = ClientWindowRect.bottom - ClientWindowRect.top;
            
            // TODO(Dustin): Resize
            //GlobalGameCode.Resize(width, height);
            
        } break;
        
        default: break;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
