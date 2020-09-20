
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <Tchar.h>
#include <strsafe.h>

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
        Platform->mprinte("FATAL ERROR: Unable to output error code.\n");
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
            Platform->mprinte("Error allocating from Platform Heap Allocator!\n");
    }
#else
    
    // TODO(Dustin): Temporary fix for not having a tagged heap/string right now
    Result = malloc(Size);
    
#endif
    
    return Result;
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

void PlatformFatalError(char *Fmt, ...)
{
    va_list Args;
    va_start(Args, Fmt);
    
    char *Message = NULL;
    int CharsRead = 1 + __Win32FormatString(Message, 1, Fmt, Args);
    Message = (char*)PlatformLocalAlloc(CharsRead);
    __Win32FormatString(Message, CharsRead, Fmt, Args);
    
    HWND *ClientWindow;
    Platform->get_client_window((platform_window**)(&ClientWindow));
    
    MessageBox(*ClientWindow, Message, "FATAL ERROR", MB_OK);
    exit(1);
}

const char* PlatformGetRequiredInstanceExtensions(bool validation_layers)
{
    VkResult err;
    VkExtensionProperties* ep;
    u32 count = 0;
    err = vk::vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
    if (err)
    {
        Platform->mprinte("Error enumerating Instance extension properties!\n");
    }
    
    ep = palloc<VkExtensionProperties>(count);
    err = vk::vkEnumerateInstanceExtensionProperties(NULL, &count, ep);
    if (err)
    {
        Platform->mprinte("Unable to retrieve enumerated extension properties!\n");
        count = 0;
    }
    
    for (u32 i = 0;  i < count;  i++)
    {
        if (strcmp(ep[i].extensionName, "VK_KHR_win32_surface") == 0)
        {
            const char *plat_exts = "VK_KHR_win32_surface";
            
            pfree(ep);
            
            return plat_exts;
        }
    }
    
    pfree(ep);
    Platform->mprinte("Could not find win32 vulkan surface extension!\n");
    
    return "";
}

void PlatformVulkanCreateSurface(VkSurfaceKHR *surface, VkInstance vulkan_instance)
{
    HWND *ClientWindow;
    Platform->get_client_window((platform_window**)(&ClientWindow));
    
    VkWin32SurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hinstance = GetModuleHandle(NULL);
    surface_info.hwnd      = *ClientWindow;
    
    VK_CHECK_RESULT(vk::vkCreateWin32SurfaceKHR(vulkan_instance, &surface_info, nullptr, surface),
                    "Unable to create XCB Surface!\n");
}
