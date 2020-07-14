// stuff will go here

/*
 
For terrain, need to specify:

 ---- Dropdown: Mesh Information
 terrain width,
 terrain height,
 
 ---- Dropdown: Simplex Noise Settings
 heightmap texture width,
 heightmap texture height,
NumOctaves    = 8;                        // Control the number of octaves used
Persistence   = 0.50f; // .5                    // Control the roughness of the output
Low           = 0.00f;                    // Low output value
High          = 1.00f;                    // Height output value
Exp           = 2.00f;                    // Controls the intensity of black to white

---- Dropdown: Thermal Erosion
Checkbox: Enable or Disabled
NumberIterations
        
---- Dropdown: Inverse Thermal Erosion
Checkbox: Enable or Disabled
NumberIterations
        
---- Dropdown: Hydraulic Erosion
Checkbox: Enable or Disabled
NumberIterations
        Rain Constant
Solubility Constant
Evaporation Coefficient
Sediment Transfer Max Coefficient

---- Button: Run Simulation
---- Button: Save Heightmap

*/

file_internal void GuiWindow(TerrainUiSettings &settings)
{
    ImGui::SetNextWindowSize(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    
    ImGuiWindowFlags window_flags = 0;
	if (!ImGui::Begin("Terrain Generation Settings", nullptr, window_flags)) // No bool flag to omit the close button
	{
		// Early out if the window is collapsed, as an optimization
		ImGui::End();
		return;
	}
    
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f); // right alifned, keep 180 pixels for the labels
    
    
    ImGui::PopItemWidth();
	ImGui::End();
}

void RenderTerrainUi(TerrainUiSettings &ui_settings, VkCommandBuffer command_buffer)
{
    // reset generation flags
    ui_settings.RecreateTerrain       = false;
    ui_settings.SaveHeightmap         = false;
    
    // Render ImGui Window
    // TODO(Dustin): Win32_NewFrame should be in the platfrom layer
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    {
        //ImGui::ShowDemoWindow();
        GuiWindow(ui_settings);
        
        // TODO(Dustin): Draw Vulkan Command - should this be in the graphics layer?
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
    }
    ImGui::EndFrame();
}