#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <libgen.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>

typedef struct
{
    xcb_connection_t  *Connection;
    xcb_screen_t      *Screen;
    xcb_window_t       Window;

    xcb_atom_t         Protocols;
    xcb_atom_t         DeleteWindow;

    xcb_key_symbols_t *KeySymbols;
} xcb_info;

file_global u32      ClientWidth;
file_global u32      ClientHeight;
file_global bool     IsRunning;
file_global xcb_info XcbInfo;

file_internal xcb_intern_atom_cookie_t XcbInternAtomCookie(xcb_connection_t *c, const char* s, u32 len);
file_internal xcb_atom_t XcbInternAtom(xcb_connection_t *c, xcb_intern_atom_cookie_t cookie);

file_internal void XcbHandleEvent(xcb_info *Xinfo, const xcb_generic_event_t *event);
file_internal void XcbLoopWait();
file_internal void XcbLoopPoll();
file_internal void XcbCreateWindow(xcb_info *Info);
file_internal void XcbInitConnection(xcb_info *Info);


file_internal xcb_intern_atom_cookie_t XcbInternAtomCookie(xcb_connection_t *c, const char* s, u32 len)
{
   return xcb_intern_atom(c, false, len, s);
}

file_internal xcb_atom_t XcbInternAtom(xcb_connection_t *c, xcb_intern_atom_cookie_t cookie)
{
    xcb_atom_t atom = XCB_ATOM_NONE;
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(c, cookie, NULL);
    if (reply) {
        atom = reply->atom;
        free(reply);
    }

    return atom;
}

file_internal void XcbInitConnection(xcb_info *Info)
{
    int scr;

    Info->Connection = xcb_connect(NULL, &scr);
    if (!Info->Connection || xcb_connection_has_error(Info->Connection)) {
        xcb_disconnect(Info->Connection);
        assert(1 && "Unable to establish a connection with the xcb server!");
    }

    const xcb_setup_t *setup = xcb_get_setup(Info->Connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0) xcb_screen_next(&iter);

    Info->Screen = iter.data;

    // Get Key symbols
    Info->KeySymbols = xcb_key_symbols_alloc(Info->Connection);
}

file_internal void XcbCreateWindow(xcb_info *Info)
{
    ClientWidth = 1080;
    ClientHeight = 720;

    Info->Window = xcb_generate_id(Info->Connection);

    u32 value_mask, value_list[32];
    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = Info->Screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(Info->Connection, XCB_COPY_FROM_PARENT, Info->Window, Info->Screen->root,
                      0, 0, ClientWidth, ClientHeight, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      Info->Screen->root_visual, value_mask, value_list);

     const char* utf8_name    = "UTF8_STRING";
     const char* ucookie_name = "WM_NAME";
     const char* uproto_name  = "WM_PROTOCOLS";
     const char* uatom_name   = "WM_DELETE_WINDOW";

     const char APP_NAME[] = "Maple Engine";

    xcb_intern_atom_cookie_t utf8_string_cookie      = XcbInternAtomCookie(Info->Connection, utf8_name, sizeof(utf8_name));
    xcb_intern_atom_cookie_t wm_name_cookie          = XcbInternAtomCookie(Info->Connection, ucookie_name, sizeof(ucookie_name));
    xcb_intern_atom_cookie_t wm_protocols_cookie     = XcbInternAtomCookie(Info->Connection, uproto_name, sizeof(uproto_name));
    xcb_intern_atom_cookie_t wm_delete_window_cookie = XcbInternAtomCookie(Info->Connection, uatom_name, sizeof(uatom_name));

    // set title
    xcb_atom_t utf8_string = XcbInternAtom(Info->Connection, utf8_string_cookie);
    xcb_atom_t wm_name     = XcbInternAtom(Info->Connection, wm_name_cookie);
    xcb_change_property(Info->Connection, XCB_PROP_MODE_REPLACE, Info->Window,
                        wm_name, utf8_string, 8, sizeof(APP_NAME), APP_NAME);

    // advertise WM_DELETE_WINDOW
    Info->Protocols    = XcbInternAtom(Info->Connection, wm_protocols_cookie);
    Info->DeleteWindow = XcbInternAtom(Info->Connection, wm_delete_window_cookie);
    xcb_change_property(Info->Connection, XCB_PROP_MODE_REPLACE, Info->Window,
                        Info->Protocols, XCB_ATOM_ATOM, 32, 1, &Info->DeleteWindow);
}

file_internal void XcbHandleEvent(xcb_info *Xinfo, const xcb_generic_event_t *ev)
{
    switch (ev->response_type & 0x7f)
    {
        case XCB_CONFIGURE_NOTIFY:
        {
            xcb_configure_notify_event_t *notify = (xcb_configure_notify_event_t*)(ev);

            // RESIZE EVENT!
            if (notify->width != ClientWidth || notify->height != ClientHeight)
            {
                if (notify->width > 0 && notify->height > 0)
                {
                    ClientWidth  = notify->width;
                    ClientHeight = notify->height;
                }
                else
                {
                    printf("Minimized...going to sleep...\n");
                    // TODO(Dustin): Do something when engine has been iconified
                }
            }
        } break;

       case XCB_KEY_PRESS:
       {
           const xcb_key_press_event_t *press = (xcb_key_press_event_t*)(ev);
           xcb_keysym_t keysym = xcb_key_symbols_get_keysym(Xinfo->KeySymbols, press->detail, 0);

            // Can find the key code translations here:
            // https://code.woboq.org/qt5/include/X11/keysymdef.h.html#210
            // or
            // /usr/include/X11/keysymdef.h

            switch (keysym) {
                case XK_Escape:
                { // escape
                    IsRunning = false;
                } break;

                case XK_Up:
                { // key up
                } break;

                case XK_Down:
                { // key down
                } break;

                case XK_Left:
                { // key up
                } break;

                case XK_Right:
                { // key down
                } break;

                case XK_space:
                { // key space
                    printf("Space was pressed!\n");
                } break;

                case XK_w:
                { // key F
                } break;

                case XK_s:
                {
                } break;

                case XK_a:
                {
                } break;

                case XK_d:
                {
                } break;

                default:
                { // key unknown
                } break;
            }
        } break;

        case XCB_CLIENT_MESSAGE:
        {
            xcb_client_message_event_t *msg = (xcb_client_message_event_t*)(ev);
            if (msg->type == Xinfo->Protocols && msg->data.data32[0] == Xinfo->DeleteWindow) IsRunning = false;
        } break;

        default: break;
   }
}

file_internal void XcbLoopWait()
{}

file_internal void XcbLoopPoll()
{}

int main(void)
{
    XcbInitConnection(&XcbInfo);
    XcbCreateWindow(&XcbInfo);

    xcb_map_window(XcbInfo.Connection, XcbInfo.Window);
    xcb_flush(XcbInfo.Connection);

    IsRunning = true;

    while (IsRunning)
    {
        while (true)
        {
            xcb_generic_event_t *ev = xcb_poll_for_event(XcbInfo.Connection);
            if (!ev) break;

            XcbHandleEvent(&XcbInfo, ev);
            free(ev); // not sure how i feel about this...
        }

        // TODO(Dustin): TIMING
    }

    xcb_destroy_window(XcbInfo.Connection, XcbInfo.Window);
    xcb_flush(XcbInfo.Connection);

    return (0);
}
