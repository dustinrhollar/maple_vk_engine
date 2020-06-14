
DEV_UI_DRAW_CALLBACK(MapleDevUi)
{
    ImGui::NewFrame();
    
#if 1
    // Render the demo window
    ImGui::ShowDemoWindow();
#endif
    
    
    
    // Render to generate draw buffers
    ImGui::Render();
    ImDrawData* ImDrawData = ImGui::GetDrawData();
    MapleDevGuiDraw(DevGui, CommandBuffer);
    ImGui::EndFrame();
}