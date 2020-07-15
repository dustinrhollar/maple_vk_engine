#ifndef ENGINE_UI_UI_H
#define ENGINE_UI_UI_H

//~ ImGui graphics initializers

struct ID3D11Device;
struct ID3D11DeviceContext;

IMGUI_IMPL_API void MapleDevGuiInit(ID3D11Device* device, ID3D11DeviceContext* device_context);
IMGUI_IMPL_API void MapleDevGuiFree();
IMGUI_IMPL_API void MapleDevGuiNewFrame();
IMGUI_IMPL_API void MapleDevGuiRenderDrawData(ImDrawData* draw_data);

// Use if you want to reset your rendering device without losing Dear ImGui state.
IMGUI_IMPL_API void MapleDevGuiInvalidateDeviceObjects();
IMGUI_IMPL_API bool MapleDevGuiCreateDeviceObjects();

//~ Window Stack

// false: window has been closed and needs to be removed from the stack
// true: just about anything else
#define UI_WINDOW_CALLBACK(fn) bool fn(struct window_stack *WindowStack, void *Data)
typedef UI_WINDOW_CALLBACK(ui_window_callback);

// if a windows resource have to be freed when it closes
// then this function must be called
#define UI_WINDOW_DATA_FREE_CALLBACK(fn) void fn(void *Data)
typedef UI_WINDOW_DATA_FREE_CALLBACK(ui_window_data_free_callback);

struct ui_window
{
    void                         *Data;
    // draw the ui window
    ui_window_callback           *Callback;
    // free any memory owned by the data
    ui_window_data_free_callback *FreeCallback;
};

#define MAX_WINDOWS 10
#define MAPLE_MAIN_UI_INDEX 0
struct window_stack
{
    ui_window Stack[MAX_WINDOWS];
    u32       WindowCount;
};

void AddWindowToStack(window_stack                 *WindowStack, 
                      ui_window_callback           *Callback, 
                      ui_window_data_free_callback *FreeCallback,
                      void                         *Data);

//~ Entry point for drawing UI.

void MapleEngineDrawUi(window_stack *WindowStack);

//~ Various UI callbacks

// EngineMainUi has an argument of a maple_ui struct 
UI_WINDOW_CALLBACK(EngineMainUi);
UI_WINDOW_DATA_FREE_CALLBACK(EngineMainUiFree);

// Test window: does not take any data
UI_WINDOW_CALLBACK(MapleDemoWindow);
UI_WINDOW_DATA_FREE_CALLBACK(MapleDemoWindowFree);


#endif //ENGINE_UI_UI_H
