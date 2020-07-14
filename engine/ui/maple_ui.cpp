file_internal void TerrainUiTab();



file_internal void TerrainUiTab(terrain_settings *Settings)
{
    if (ImGui::CollapsingHeader("Mesh Settings"))
    {
        if (ImGui::InputInt("Terrain Width", (int*)&Settings->TerrainWidth))
        {
            Settings->TerrainMeshUpdated    = true;
        }
        
        if (ImGui::InputInt("Terrain Height", (int*)&Settings->TerrainHeight))
        {
            Settings->TerrainMeshUpdated    = true;
        }
    }
    
    if (ImGui::CollapsingHeader("Simplex Simulation Settings"))
    {
        if (ImGui::InputInt("Heightmap Width", (int*)&Settings->HeightmapWidth))
        {
            Settings->HeightmapUpdated     = true;
        }
        
        if (ImGui::InputInt("Heightmap Height", (int*)&Settings->HeightmapHeight))
        {
            Settings->HeightmapUpdated     = true;
        }
        
        if (ImGui::InputInt("Number Of Octaves", (int*)&Settings->NumberOfOctaves, 1, 1))
        {
        }
        
        if (ImGui::DragFloat("Persistence", &Settings->Persistence, 0.01f, 0.0f))
        {
        }
        
        if (ImGui::DragFloat("Low", &Settings->Low, 0.01f, 0.0f))
        {
        }
        
        if (ImGui::DragFloat("High", &Settings->High, 0.01f, 0.0f))
        {
        }
        
        if (ImGui::DragFloat("Exponent", &Settings->Exp, 0.01f, 0.0f))
        {
        }
    }
    
    if (ImGui::CollapsingHeader("Thermal Erosion Settings"))
    {
        if (ImGui::Checkbox("Active", &Settings->ThermalEnabled))
        {
        }
        
        if (ImGui::InputInt("Number Of Iterations", (int*)&Settings->ThermalNumIterations, 1, 1))
        {
        }
    }
    
    if (ImGui::CollapsingHeader("Inverse Thermal Erosion Settings"))
    {
        if (ImGui::Checkbox("Active", &Settings->InverseThermalEnabled))
        {
        }
        
        if (ImGui::InputInt("Number Of Iterations", (int*)&Settings->InverseThermalNumIterations, 1, 1))
        {
        }
    }
    
    if (ImGui::CollapsingHeader("Hydraulic Erosion Settings"))
    {
        if (ImGui::Checkbox("Active", &Settings->HydraulicEnabled))
        {
        }
        
        if (ImGui::InputInt("Number Of Iterations", (int*)&Settings->HydraulicNumIterations, 1, 1))
        {
        }
        
        if (ImGui::DragFloat("Rain Constant", &Settings->RainConstant, 0.01f, 0.0f))
        {
        }
        
        if (ImGui::DragFloat("Solubility Constant", &Settings->SolubilityConstant, 0.01f, 0.0f))
        {
        }
        
        if (ImGui::DragFloat("Evaporation Coefficient", &Settings->EvaporationCoefficient, 0.01f, 0.0f))
        {
        }
        
        if (ImGui::DragFloat("Sediment Transfer Maximum Coefficient", &Settings->SedimentTransferMaxCoefficient, 0.01f, 0.0f))
        {
        }
    }
    
    if (ImGui::Button("Generate Terrain"))
    {
        Settings->RecreateTerrain = true;
    }
    
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::Button("Save Terrain Heightmap"))
    {
        ImGui::OpenPopup("Save Heightmap");
        io.WantCaptureKeyboard = true;
    }
    
    ImVec2 WindowSize = { 300, 150 };
    ImGui::SetNextWindowSize(WindowSize, 0);
    if (ImGui::BeginPopupModal("Save Heightmap", NULL, ImGuiWindowFlags_MenuBar))
    {
        static char buf[256];
        
        ImGui::InputText("Filename: ", buf, sizeof(buf));
        
        if (ImGui::Button("Save"))
        {
            mprint("Filename: \"%s\"\n", buf);
            
            MstringFree(&Settings->SaveFilename);
            Mstring(&Settings->SaveFilename, buf, strlen(buf));
            Settings->SaveHeightmap = true;
            
            ImGui::CloseCurrentPopup();
        }
        
        if (ImGui::Button("Back"))
        {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void MapleEngineUiDraw(maple_ui *Ui)
{
    // Draw dev gui
    MapleDevGuiNewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    {
        
        // TODO(Dustin): Create tab structure
        //ImGui::ShowDemoWindow();
        TerrainUiTab(&Ui->TerrainSettings);
        
    }
    ImGui::Render();
    ImDrawData* ImDrawData = ImGui::GetDrawData();
    MapleDevGuiRenderDrawData(ImDrawData);
    ImGui::EndFrame();
}