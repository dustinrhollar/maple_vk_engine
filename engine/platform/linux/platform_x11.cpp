
file_global u32 ClientWidth;
file_global u32 ClientHeight;
file_global r32 ClientMouseXPos;
file_global r32 ClientMouseYPos;
file_global bool GlobalIsDevMode;
file_global GLFWwindow *ClientWindow;

file_internal void WindowResize(GLFWwindow* window, int width, int height);
file_internal void MouseCallback(GLFWwindow* window, double xpos, double ypos);
file_internal u32 ProcessUserInput(GLFWwindow *window);

jstring X11GetExeFilepath()
{
    jstring result;
    InitJString(&result);
    
    return result;
}

DynamicArray<jstring> X11FindAllFilesInDirectory(jstring &directory, jstring delimiter)
{
    DynamicArray<jstring> result;
    return result;
}

void X11WriteBufferToFile(jstring &file, void *buffer, u32 size)
{
}

jstring X11LoadFile(jstring &directory, jstring &filename)
{
    jstring result;
    InitJString(&result);
    
    return result;
}

void X11DeleteFile(jstringng &file)
{
}

void X11ExecuteCommand(jstring &system_cmd)
{
}

void X11EnableLogging()
{
}

const char *X11GetRequiredInstanceExtensions(bool validation_layers)
{
    const char *result = "VK_KHR_xlib_surface";
    return result;
}

void X11GetClientWindowDimensionsb(u32 *width, u32 *height)
{
    *width = ClientWidth;
    *height = ClientHeight;
}

void X11VulkanCreateSurface(VkSurfaceKHR *surface, VkInstance vulkan_instance)
{
    glfwCreateWindowSurface(vulkan_instance, ClientWindow, nullptr, surface);
}

file_internal void CreateClientWindow()
{
    // TODO(Dustin): Use X11 directly rather than relying on GLFW
    ClientWidth = 800;
    ClientHeight = 600;
    
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    
    ClientWindow = glfwCreateWindow(ClientWidth, ClientHeight, "Example", NULL, NULL);
    if (ClientWindow == NULL)
    {
        printf("Failed to create window!\n");
        exit(1);
    }
    glfwMakeContextCurrent(ClientWindow);
    glfwSetCursorPosCallback(ClientWindow, MouseCallback);
    glfwSetFramebufferSizeCallback(ClientWindow, WindowResize);
}

file_internal void StartupRoutines()
{
    X11EnableLogging();
    
    // Initialize Management Routines
    // NOTE(Dustin): Macros ar defined in engine_config.h
    mm::InitializeMemoryManager(MEMORY_USAGE, TRANSIENT_MEMORY);
    ecs::InitializeECS();
    
    // Create the client windows
    CreateClientWindow();
    
    // Enable graphics
    if (!vk::InitializeVulkan())
    {
        printf("Unable to initialize Vulkan!\n");
        glfwTerminate();
        ecs::ShutdownECS();
        mm::ShutdownMemoryManager();
        
        exit(1);
    }
}

file_internal void ShutdownRoutines()
{
    vk::ShutdownVulkan();
    glfwTerminate();
    ecs::ShutdownECS();
    mm::ShutdownMemoryManager();
}

file_internal void WindowResize(GLFWwindow* window, int width, int height)
{
    ClientWidth = width;
    ClientHeight = height;
}

file_internal void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    ClientMouseXPos = (r32)xpos;
    ClientMouseYPos = (r32)ypos;
}

file_internal u32 ProcessUserInput(GLFWwindow *window)
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
int main()
{
    StartupRoutines();
    GameInit();
    
    
    while (!glfwWindowShouldClose(ClientWindow))
    {
        u32 frame_input = ProcessUserInput(ClientWindow);
        
        glfwSwapBuffers(ClientWindow);
        glfwPollEvents();
    }
    
    return (0);
}
