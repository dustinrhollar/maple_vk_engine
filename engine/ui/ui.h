#ifndef ENGINE_UI_UI_H
#define ENGINE_UI_UI_H

struct ID3D11Device;
struct ID3D11DeviceContext;

IMGUI_IMPL_API void MapleDevGuiInit(ID3D11Device* device, ID3D11DeviceContext* device_context);
IMGUI_IMPL_API void MapleDevGuiFree();
IMGUI_IMPL_API void MapleDevGuiNewFrame();
IMGUI_IMPL_API void MapleDevGuiRenderDrawData(ImDrawData* draw_data);

// Use if you want to reset your rendering device without losing Dear ImGui state.
IMGUI_IMPL_API void MapleDevGuiInvalidateDeviceObjects();
IMGUI_IMPL_API bool MapleDevGuiCreateDeviceObjects();

#endif //ENGINE_UI_UI_H
