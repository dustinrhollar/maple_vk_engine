
#include <xcb/xcb.h>
#include <X11/Xlib.h>

#include <libgen.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>

file_global u32 ClientWidth;
file_global u32 ClientHeight;
file_global bool IsRunning;

file_internal xcb_intern_atom_cookie_t intern_atom_cookie(xcb_connection_t *c, const jstring s);
file_internal xcb_atom_t intern_atom(xcb_connection_t *c, xcb_intern_atom_cookie_t cookie);

file_internal void XcbHandleEvent(const xcb_generic_event_t *event);
file_internal void XcbLoopWait();
file_internal void XcbLoopPoll();
file_internal void XcbCreateWindow();
file_internal void XcbInitConnection();

struct XCBInfo
{
    xcb_connection_t *Connection;
    xcb_screen_t *Screen;
    xcb_window_t Window;
    
    xcb_atom_t Protocols;
    xcb_atom_t DeleteWindow;
} xcb_info;

jstring XcbGetExeFilepath()
{
    jstring result;
    char exe_filename[ PATH_MAX ];
    ssize_t count = readlink( "/proc/self/exe", exe_filename, PATH_MAX );
    
    if (count > 0)
    {
        char *exe_path = dirname(exe_filename);
        
        printf("EXE path: %s\n", exe_path);
        
        result = InitJString(exe_path);
        result += '/';
    }
    
    return result;
}

DynamicArray<jstring> XcbFindAllFilesInDirectory(jstring &directory, jstring delimiter)
{
    DynamicArray<jstring> result;
    return result;
}

void XcbWriteBufferToFile(jstring &file, void *buffer, u32 size)
{
}

jstring XcbLoadFile(jstring &directory, jstring &filename)
{
    jstring result = {};
    
    jstring fullpath = directory + filename;
    int fd = open(fullpath.GetCStr(), O_RDONLY);
    if (fd > 0)
    {
        // determine file size
        struct stat st;
        fstat(fd, &st);
        int size = st.st_size;
        
        char *buf = talloc<char>(size + 1);
        ssize_t read_bytes = read(fd, buf, size);
        assert(read_bytes == size);
        
        result = InitJString(buf, size);
    }
    
    return result;
}

void XcbDeleteFile(jstring &file)
{
}

void XcbExecuteCommand(jstring &system_cmd)
{
}

void XcbEnableLogging()
{
}

const char *XcbGetRequiredInstanceExtensions(bool validation_layers)
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
    }
    
    bool xlib_found = false;
    bool xcb_found = false;
    for (u32 i = 0;  i < count;  i++)
    {
        if (strcmp(ep[i].extensionName, "VK_KHR_xlib_surface") == 0)
        {
            xlib_found = true;
        }
        else if (strcmp(ep[i].extensionName, "VK_KHR_xcb_surface") == 0)
        {
            xcb_found = true;
        }
    }
    
    pfree(ep);
    
    if (xlib_found && xcb_found)
    {
        const char *plat_exts = "VK_KHR_xcb_surface";
        return plat_exts;
    }
    else
    {
        const char *plat_exts = "VK_KHR_xlib_surface";
        return plat_exts;
    }
}

void XcbGetClientWindowDimensions(u32 *width, u32 *height)
{
    *width  = ClientWidth;
    *height = ClientHeight;
}

void XcbVulkanCreateSurface(VkSurfaceKHR *surface, VkInstance vulkan_instance)
{
    VkXcbSurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surface_info.connection = xcb_info.Connection;
    surface_info.window = xcb_info.Window;
    
    VK_CHECK_RESULT(vk::vkCreateXcbSurfaceKHR(vulkan_instance, &surface_info, nullptr, surface),
                    "Unable to create XCB Surface!\n");
}

file_internal void StartupRoutines()
{
    XcbEnableLogging();
    
    // Initialize Management Routines
    // NOTE(Dustin): Macros ar defined in engine_config.h
    mm::InitializeMemoryManager(MEMORY_USAGE, TRANSIENT_MEMORY);
    ecs::InitializeECS();
    
    // Init XCB
    XcbInitConnection();
    XcbCreateWindow();
    
    // Enable graphics
    if (!vk::InitializeVulkan())
    {
        printf("Unable to initialize Vulkan!\n");
        //glfwTerminate();
        ecs::ShutdownECS();
        mm::ShutdownMemoryManager();
        
        exit(1);
    }
}


file_internal void ShutdownRoutines()
{
    vk::ShutdownVulkan();
    ecs::ShutdownECS();
    mm::ShutdownMemoryManager();
}

file_internal void XcbHandleEvent(const xcb_generic_event_t *ev)
{
    switch (ev->response_type & 0x7f) {
        case XCB_CONFIGURE_NOTIFY:
        {
            const xcb_configure_notify_event_t *notify = reinterpret_cast<const xcb_configure_notify_event_t *>(ev);
            
            // RESIZE EVENT!
            if (notify->width != ClientWidth || notify->height != ClientHeight)
            {
                if (notify->width > 0 && notify->height > 0)
                {
                    ClientWidth  = notify->width;
                    ClientHeight = notify->height;
                    FlagGameResize();
                }
                else
                {
                    printf("Minimized...going to sleep...\n");
                    XcbLoopWait();
                }
            }
        } break;
        
        case XCB_KEY_PRESS:
        {
            const xcb_key_press_event_t *press = reinterpret_cast<const xcb_key_press_event_t *>(ev);
            
            // TODO translate xcb_keycode_t
            switch (press->detail) {
                case 9:
                { // escape
                    IsRunning = false;
                } break;
                
                case 111:
                { // key up
                } break;
                
                case 116:
                { // key down
                } break;
                
                case 65:
                { // key space
                } break;
                
                case 41:
                { // key F
                } break;
                
                default:
                { // key unknown
                } break;
            }
        } break;
        case XCB_CLIENT_MESSAGE:
        {
            const xcb_client_message_event_t *msg = reinterpret_cast<const xcb_client_message_event_t *>(ev);
            if (msg->type == xcb_info.Protocols && msg->data.data32[0] == xcb_info.DeleteWindow) IsRunning = false;
        } break;
        
        default: break;
    }
}

file_internal void XcbLoopWait()
{
    while (1)
    {
        xcb_generic_event_t *ev = xcb_wait_for_event(xcb_info.Connection);
        
        // TODO(Dustin): Loop wait occurs when a window is minimized?
        // wait for an event to appear in the queue - if it is a resize
        // request see if the user maximized the window
        switch (ev->response_type & 0x7f) {
            case XCB_CONFIGURE_NOTIFY:
            {
                const xcb_configure_notify_event_t *notify = reinterpret_cast<const xcb_configure_notify_event_t *>(ev);
                
                if (notify->width > 0 && notify->height > 0)
                {
                    printf("Waking up...\n");
                    
                    ClientWidth  = notify->width;
                    ClientHeight = notify->height;
                    FlagGameResize();
                    return;
                }
            } break;
            
            default: break;
        }
        
        free(ev);
    }
}

file_internal xcb_intern_atom_cookie_t intern_atom_cookie(xcb_connection_t *c, const jstring s)
{
    return xcb_intern_atom(c, false, s.len, s.GetCStr());
}

file_internal xcb_atom_t intern_atom(xcb_connection_t *c, xcb_intern_atom_cookie_t cookie)
{
    xcb_atom_t atom = XCB_ATOM_NONE;
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(c, cookie, nullptr);
    if (reply) {
        atom = reply->atom;
        free(reply);
    }
    
    return atom;
}

file_internal void XcbCreateWindow()
{
    ClientWidth = 800;
    ClientHeight = 600;
    
    xcb_info.Window = xcb_generate_id(xcb_info.Connection);
    
    u32 value_mask, value_list[32];
    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = xcb_info.Screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    
    xcb_create_window(xcb_info.Connection, XCB_COPY_FROM_PARENT, xcb_info.Window, xcb_info.Screen->root,
                      0, 0, ClientWidth, ClientHeight, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      xcb_info.Screen->root_visual, value_mask, value_list);
    
    jstring utf8_name   = InitJString("UTF8_STRING");
    jstring cookie_name = InitJString("WM_NAME");
    jstring proto_name  = InitJString("WM_PROTOCOLS");
    jstring atom_name   = InitJString("WM_DELETE_WINDOW");
    
    xcb_intern_atom_cookie_t utf8_string_cookie = intern_atom_cookie(xcb_info.Connection, utf8_name);
    xcb_intern_atom_cookie_t wm_name_cookie = intern_atom_cookie(xcb_info.Connection, cookie_name);
    xcb_intern_atom_cookie_t wm_protocols_cookie = intern_atom_cookie(xcb_info.Connection, proto_name);
    xcb_intern_atom_cookie_t wm_delete_window_cookie = intern_atom_cookie(xcb_info.Connection, atom_name);
    
    // set title
    xcb_atom_t utf8_string = intern_atom(xcb_info.Connection, utf8_string_cookie);
    xcb_atom_t wm_name = intern_atom(xcb_info.Connection, wm_name_cookie);
    xcb_change_property(xcb_info.Connection, XCB_PROP_MODE_REPLACE, xcb_info.Window,
                        wm_name, utf8_string, 8, sizeof(APP_NAME), APP_NAME);
    
    // advertise WM_DELETE_WINDOW
    xcb_info.Protocols    = intern_atom(xcb_info.Connection, wm_protocols_cookie);
    xcb_info.DeleteWindow = intern_atom(xcb_info.Connection, wm_delete_window_cookie);
    xcb_change_property(xcb_info.Connection, XCB_PROP_MODE_REPLACE, xcb_info.Window,
                        xcb_info.Protocols, XCB_ATOM_ATOM, 32, 1, &xcb_info.DeleteWindow);
    
    
    utf8_name.Clear();
    cookie_name.Clear();
    proto_name.Clear();
    atom_name.Clear();
}

file_internal void XcbInitConnection()
{
    int scr;
    
    xcb_info.Connection = xcb_connect(nullptr, &scr);
    if (!xcb_info.Connection || xcb_connection_has_error(xcb_info.Connection)) {
        xcb_disconnect(xcb_info.Connection);
        throw std::runtime_error("failed to connect to the display server");
    }
    
    const xcb_setup_t *setup = xcb_get_setup(xcb_info.Connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0) xcb_screen_next(&iter);
    
    xcb_info.Screen = iter.data;
}

int main()
{
    IsRunning = false;
    
    StartupRoutines();
    GameInit();
    
    xcb_map_window(xcb_info.Connection, xcb_info.Window);
    xcb_flush(xcb_info.Connection);
    
    IsRunning = true;
    while (IsRunning)
    {
        while (true)
        {
            xcb_generic_event_t *ev = xcb_poll_for_event(xcb_info.Connection);
            if (!ev) break;
            
            XcbHandleEvent(ev);
            free(ev); // not sure how i feel about this...
        }
        
        GameUpdateAndRender();
        
        // TODO(Dustin): TIMING
    }
    
    GameShutdown();
    ShutdownRoutines();
    
    xcb_destroy_window(xcb_info.Connection, xcb_info.Window);
    xcb_flush(xcb_info.Connection);
    
    return (0);
}
