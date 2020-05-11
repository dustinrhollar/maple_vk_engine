#ifndef ENGINE_EVENTS_H
#define ENGINE_EVENTS_H

// Engine events are pre-defined events that a client can
// register to.

// TODO(Dustin): Subscribe to specific key press events

#define BIT(x) 1<<(x)

enum EventKey
{
    KEY_a,
    KEY_b,
    KEY_c,
    KEY_d,
    KEY_e,
    KEY_f,
    KEY_g,
    KEY_h,
    KEY_i,
    KEY_j,
    KEY_k,
    KEY_l,
    KEY_m,
    KEY_n,
    KEY_o,
    KEY_p,
    KEY_q,
    KEY_r,
    KEY_s,
    KEY_t,
    KEY_u,
    KEY_v,
    KEY_w,
    KEY_x,
    KEY_y,
    KEY_z,
    
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    
    KEY_Unknown,
    
    KEY_Count,
};

enum EventType
{
    EVENT_TYPE_ON_MOUSE_BUTTON_PRESS = 0,
    EVENT_TYPE_ON_MOUSE_BUTTON_RELEASE,
    EVENT_TYPE_ON_MOUSE_MOVE,
    EVENT_TYPE_ON_KEY_PRESS,
    EVENT_TYPE_ON_KEY_RELEASE,
    EVENT_TYPE_ON_KEY_TYPE_PRESS,
    EVENT_TYPE_ON_WINDOW_RESIZE,
    
    EVENT_TYPE_CUSTOM,
    
    EVENT_TYPE_COUNT,
};

struct MouseMoveEvent
{
    i32 x;
    i32 y;
};

struct MouseButtonPressEvent
{
};

struct MouseButtonReleaseEvent
{
};

struct KeyPressEvent
{
};

struct KeyReleaseEvent
{
};

// KeyTypePress/Release allow a user to specify a particular button
// to press
struct KeyTypePressEvent
{
    EventKey Key;
};

struct KeyTypeReleaseEvent
{
    EventKey Key;
};

struct WindowResizeEvent
{
    u32 Width;
    u32 Height;
};

struct CoreVulkanResizeEvent
{
    u32 Width;
    u32 Height;
};

struct ResizeEvent
{
    u32 Width;
    u32 Height;
};

#endif //ENGINE_EVENTS_H
