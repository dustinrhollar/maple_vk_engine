#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <vulkan/vulkan.h>

#include <stdio.h>

#include <jackal_types.h>
#include "platform.h"
#include "logger.h"
#include "GLFW/glfw3.h"

#include "logger.cpp"

// Timing information
file_global GameApi GlobalGameCode;
file_global i64 GlobalPerfCountFrequency;

// Window information
file_global GLFWwindow *GlobalWindow;
file_global u32 GlobalClientWidth;
file_global u32 GlobalClientHeight;

// Mouse information
file_global r32 GlobalMouseXPos;
file_global r32 GlobalMouseYPos;

// Game or Dev Mode?
file_global bool GlobalIsDevMode = true;

struct CodeDLL
{
    HMODULE game;
} GlobalCodeDLL;

void WindowResize(GLFWwindow* window, int width, int height);
void MouseCallback(GLFWwindow* window, double xpos, double ypos);
// Returns a bitfield containing all of the important key presses
u32 ProcessUserInput(GLFWwindow *window);

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

PLATFORM_WAIT_FOR_EVENTS(Win32WaitForEvents)
{
    int width = 0, height = 0;
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(GlobalWindow, &width, &height);
        glfwWaitEvents();
    }
}

PLATFORM_GET_WALL_CLOCK(Win32GetWallClock)
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return (Result.QuadPart);
    
}

PLATFORM_GET_SECONDS_ELAPSED(Win32GetSecondsElapsed)
{
    r32 Result = ((r32)(end - start) /
                  (r32)GlobalPerfCountFrequency);
    return(Result);
}

PLATFORM_GET_REQUIRED_INSTANCE_EXTENSIONS(Win32GetRequiredInstanceExtensions)
{
    u32 glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    
    std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    
    const char *plat_exts = "VK_KHR_win32_surface";
    return plat_exts;
};

PLATFORM_CREATE_SURFACE(Win32CreateVulkanSurface)
{
    glfwCreateWindowSurface(instance, GlobalWindow, nullptr, surface);
}

PLATFORM_GET_CLIENT_WINDOW_DIMENSIONS(Win32GetClientWindowDimensions)
{
    glfwGetFramebufferSize(GlobalWindow, width, height);
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR lpCmdLine, INT nCmdShow)
{
    // Setup timing information
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    
    // Initialize the Logger
    InitializeLogger();
    
    UINT desired_scheduler_ms = 1;
    bool sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
    
    platform_api api = {};
    api.GetWallClock                  = Win32GetWallClock;
    api.GetSecondsElapsed             = Win32GetSecondsElapsed;
    api.GetRequiredInstanceExtensions = Win32GetRequiredInstanceExtensions;
    api.CreateSurface                 = Win32CreateVulkanSurface;
    api.GetClientWindowDimensions     = Win32GetClientWindowDimensions;
    api.WaitForEvents                 = Win32WaitForEvents;
    
    const char *game_lib = "libsplicer.dll";
    GlobalGameCode = LoadGameApi(game_lib);
    
    if (!GlobalGameCode.is_valid)
    {
        printf("Unable to load game code!\n");
        return (1);
    }
    
    glfwInit();
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    
    GlobalClientWidth = 800;
    GlobalClientHeight = 600;
    
    GlobalWindow = glfwCreateWindow(GlobalClientWidth, GlobalClientHeight, "Splicer", NULL, NULL);
    if (GlobalWindow == NULL)
    {
        printf("Failed to create window!\n");
        return (1);
    }
    glfwMakeContextCurrent(GlobalWindow);
    glfwSetCursorPosCallback(GlobalWindow, MouseCallback);
    
    if (!GlobalGameCode.Initialize(api, (void*)glfwGetProcAddress))
        return (1);
    
    GlobalGameCode.Resize(GlobalClientWidth, GlobalClientHeight);
    glfwSetFramebufferSizeCallback(GlobalWindow, WindowResize);
    
    u64 last_frame_time = Win32GetWallClock();
    r32 refresh_rate = 60.0f;
    r32 target_seconds_per_frame = 1 / refresh_rate;
    
    while (!glfwWindowShouldClose(GlobalWindow))
    {
        GameState game_state = {};
        game_state.input_bitfield = ProcessUserInput(GlobalWindow);
        game_state.mouse_xpos = GlobalMouseXPos;
        game_state.mouse_ypos = GlobalMouseYPos;
        game_state.DevMode = GlobalIsDevMode;
        
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
        
        //printf("Frame rate: %lf ms\n", 1000.0f * seconds_elapsed_per_frame);
        
        game_state.time = seconds_elapsed_per_frame;
        GlobalGameCode.Update(game_state);
        
        glfwSwapBuffers(GlobalWindow);
        glfwPollEvents();
    }
    
    GlobalGameCode.Shutdown();
    glfwTerminate();
    
    return (0);
}


void WindowResize(GLFWwindow* window, int width, int height)
{
    GlobalGameCode.Resize(width, height);
    GlobalClientWidth = width;
    GlobalClientHeight = height;
}

void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    GlobalMouseXPos = (float)xpos;
    GlobalMouseYPos = (float)ypos;
}

u32 ProcessUserInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    { // close the application
        glfwSetWindowShouldClose(window, true);
    }
    
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
    { // Capture mouse mode - "game mode"
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        GlobalIsDevMode = false;
    }
    
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
    { // do not capture mouse - "dev mode"
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        GlobalIsDevMode = true;
    }
    
    // GAME / ENGINE Input
    u32 bitfield = 0;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        bitfield |= KEY_PRESS_W;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        bitfield |= KEY_PRESS_S;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        bitfield |= KEY_PRESS_A;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        bitfield |= KEY_PRESS_D;
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        bitfield |= KEY_PRESS_1;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        bitfield |= KEY_PRESS_2;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        bitfield |= KEY_PRESS_3;
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        bitfield |= KEY_PRESS_4;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        bitfield |= KEY_PRESS_UP;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        bitfield |= KEY_PRESS_DOWN;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        bitfield |= KEY_PRESS_LEFT;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        bitfield |= KEY_PRESS_RIGHT;
    
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        bitfield |= KEY_PRESS_I;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        bitfield |= KEY_PRESS_O;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        bitfield |= KEY_PRESS_P;
    
    return bitfield;
}
