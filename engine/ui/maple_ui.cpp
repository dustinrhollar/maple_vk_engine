
file_global const char *TerrainImageDir = "data/terrain/";

file_internal void TerrainUiTab();
file_internal void SaveHeightmapToFile(const char *Filename);

void AddWindowToStack(window_stack                 *WindowStack, 
                      ui_window_callback           *Callback, 
                      ui_window_data_free_callback *FreeCallback,
                      void                         *Data)
{
    if (WindowStack->WindowCount + 1 > MAX_WINDOWS)
    {
        mprinte("Unable to create new window! Max windows has been reached!");
    }
    else
    {
        u32 ActualIndex = WindowStack->WindowCount;
        
        if (!WindowStack->Stack[MAPLE_MAIN_UI_INDEX].Callback)
        { // main ui has been destroyed, so adjust the index appropriately
            ActualIndex += MAPLE_MAIN_UI_INDEX + 1;
        }
        
        WindowStack->Stack[ActualIndex].Data         = Data;
        WindowStack->Stack[ActualIndex].Callback     = Callback;
        WindowStack->Stack[ActualIndex].FreeCallback = FreeCallback;
        WindowStack->WindowCount++;
    }
}

file_internal void SaveHeightmapToFile(const char *Filename)
{
    asset_terrain Terrain = GetAsset(GlobalAssetRegistry.TerrainId)->Terrain;
    
    if (Terrain.Heightmap)
    {
        mstring Path = {};
        StringAdd(&Path, TerrainImageDir, strlen(TerrainImageDir), Filename, strlen(Filename));
        mprint("Terrain filepath: %s\n", GetStr(&Path));
        
        file_t File = PlatformOpenFile(GetStr(&Path));
        
        int Length = Terrain.HeightmapWidth * Terrain.HeightmapHeight;
        PlatformWriteToFile(File, "P6\n%d %d\n255\n",
                            Terrain.HeightmapWidth, Terrain.HeightmapHeight);
        
        for (int k = 0; k < Length; ++k)
        {
            u8 n = Terrain.Heightmap[k] * 255;
            PlatformWriteToFile(File, "%c%c%c",n, n, n);
        }
        
        PlatformFlushFile(File);
        PlatformCloseFile(File);
        MstringFree(&Path);
    }
}

file_internal void TerrainUiTab(terrain_settings *Settings)
{
    if (ImGui::CollapsingHeader("Terrain Generator"))
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
                Settings->HeightmapUpdated     = true;
            }
            
            if (ImGui::DragFloat("Persistence", &Settings->Persistence, 0.01f, 0.0f))
            {
                Settings->HeightmapUpdated     = true;
            }
            
            if (ImGui::DragFloat("Low", &Settings->Low, 0.01f, 0.0f))
            {
                Settings->HeightmapUpdated     = true;
            }
            
            if (ImGui::DragFloat("High", &Settings->High, 0.01f, 0.0f))
            {
                Settings->HeightmapUpdated     = true;
            }
            
            if (ImGui::DragFloat("Exponent", &Settings->Exp, 0.01f, 0.0f))
            {
                Settings->HeightmapUpdated     = true;
            }
        }
        
        if (ImGui::CollapsingHeader("Thermal Erosion Settings"))
        {
            if (ImGui::Checkbox("Active", &Settings->ThermalEnabled))
            {
                Settings->HeightmapUpdated     = true;
            }
            
            if (ImGui::InputInt("Number Of Iterations", (int*)&Settings->ThermalNumIterations, 1, 1))
            {
                Settings->HeightmapUpdated     = true;
            }
        }
        
        if (ImGui::CollapsingHeader("Inverse Thermal Erosion Settings"))
        {
            if (ImGui::Checkbox("Active", &Settings->InverseThermalEnabled))
            {
                Settings->HeightmapUpdated     = true;
            }
            
            if (ImGui::InputInt("Number Of Iterations", (int*)&Settings->InverseThermalNumIterations, 1, 1))
            {
                Settings->HeightmapUpdated     = true;
            }
        }
        
        if (ImGui::CollapsingHeader("Hydraulic Erosion Settings"))
        {
            if (ImGui::Checkbox("Active", &Settings->HydraulicEnabled))
            {
                Settings->HeightmapUpdated     = true;
            }
            
            if (ImGui::InputInt("Number Of Iterations", (int*)&Settings->HydraulicNumIterations, 1, 1))
            {
                Settings->HeightmapUpdated     = true;
            }
            
            if (ImGui::DragFloat("Rain Constant", &Settings->RainConstant, 0.01f, 0.0f))
            {
                Settings->HeightmapUpdated     = true;
            }
            
            if (ImGui::DragFloat("Solubility Constant", &Settings->SolubilityConstant, 0.01f, 0.0f))
            {
                Settings->HeightmapUpdated     = true;
            }
            
            if (ImGui::DragFloat("Evaporation Coefficient", &Settings->EvaporationCoefficient, 0.01f, 0.0f))
            {
                Settings->HeightmapUpdated     = true;
            }
            
            if (ImGui::DragFloat("Sediment Transfer Maximum Coefficient", &Settings->SedimentTransferMaxCoefficient, 0.01f, 0.0f))
            {
                Settings->HeightmapUpdated     = true;
            }
        }
        
        if (ImGui::Button("Generate Terrain"))
        {
            if (!IsValidAsset(GlobalAssetRegistry.TerrainId))
                CreateTerrain(&GlobalAssetRegistry, Settings);
            else
            {
                asset *TerrainAsset = GetAsset(GlobalAssetRegistry.TerrainId);
                ResetTerrain(&TerrainAsset->Terrain, Settings);
            }
            
            Settings->TerrainGenerated = true;
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
                
                SaveHeightmapToFile(buf);
                
                // TODO(Dustin): Colormap, normalmap, etc.
                
                ImGui::CloseCurrentPopup();
            }
            
            if (ImGui::Button("Back"))
            {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        
        io = ImGui::GetIO();
        if (ImGui::Button("Load Terrain"))
        {
            ImGui::OpenPopup("Load Terrain");
            io.WantCaptureKeyboard = true;
        }
        
        ImGui::SetNextWindowSize(WindowSize, 0);
        if (ImGui::BeginPopupModal("Load Terrain", NULL, ImGuiWindowFlags_MenuBar))
        {
            
            ImGuiIO& io = ImGui::GetIO();
            if (ImGui::Button("Load Heightmap File"))
            {
                ImGui::CloseCurrentPopup();
                ImGui::OpenPopup("Load Thing");
                io.WantCaptureKeyboard = true;
            }
            
            // TODO(Dustin): Load normalmap, colormap, etc.
            
            // TODO(Dustin): Load via file chooser instead of typing filename
            
            
            if (ImGui::Button("Back"))
            {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        
        // TODO(Dustin): Popup not pulling up...!
        ImGui::SetNextWindowSize(WindowSize, 0);
        if (ImGui::BeginPopupModal("Load Thing", NULL, ImGuiWindowFlags_MenuBar))
        {
            static char buf[256];
            
            ImGui::InputText("Filename: ", buf, sizeof(buf));
            
            if (ImGui::Button("Load"))
            {
                mprint("Filename: \"%s\"\n", buf);
                
                // TODO(Dustin): Load the heightmap from file
                
                ImGui::CloseCurrentPopup();
            }
            
            if (ImGui::Button("Back"))
            {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }
    
    if (ImGui::CollapsingHeader("View Terrain Textures"))
    {
        if (Settings->TerrainGenerated)
        {
            asset_terrain *Terrain = &(GetAsset(GlobalAssetRegistry.TerrainId)->Terrain);
            
            // show the terrain heightmap 
            {
                ImGuiIO& io = ImGui::GetIO();
                resource_texture HeightmapTexture = 
                    GlobalResourceRegistry->Resources[Terrain->HeightmapTexture.Index]->Texture;
                
                ImTextureID my_tex_id = (ImTextureID)HeightmapTexture.View;
                float my_tex_w = (float)HeightmapTexture.Width;
                float my_tex_h = (float)HeightmapTexture.Height;
                
                int frame_padding = 0;                            // -1 == uses default padding (style.FramePadding)
                ImVec2 size = ImVec2(200.0f, 200.0f);             // Size of the image we want to make visible
                ImVec2 uv0 = ImVec2(0.0f, 0.0f);                  // Top-left
                ImVec2 uv1 = ImVec2(1.0f, 1.0f);                  // Lower-right
                ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);   // Black background
                ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
                if (ImGui::ImageButton(my_tex_id, size, uv0, uv1, frame_padding, bg_col, tint_col))
                {
                }
            }
            
            // show the terrain colormap
            {
                resource_texture ColormapTexture = 
                    GlobalResourceRegistry->Resources[Terrain->ColormapTexture.Index]->Texture;
                
                ImTextureID ColormapId = (ImTextureID)ColormapTexture.View;
                float my_tex_w = (float)ColormapTexture.Width;
                float my_tex_h = (float)ColormapTexture.Height;
                
                int frame_padding = 0;                            // -1 == uses default padding (style.FramePadding)
                ImVec2 size = ImVec2(200.0f, 200.0f);             // Size of the image we want to make visible
                ImVec2 uv0 = ImVec2(0.0f, 0.0f);                  // Top-left
                ImVec2 uv1 = ImVec2(1.0f, 1.0f);                  // Lower-right
                ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);   // Black background
                ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
                if (ImGui::ImageButton(ColormapId, size, uv0, uv1, frame_padding, bg_col, tint_col))
                {
                }
            }
        }
    }
    
}

UI_WINDOW_CALLBACK(EngineMainUi)
{
    bool Result = true;
    
    maple_ui *Ui = (maple_ui*)Data;
    
    static bool show_app_about = false;
    if (!ImGui::Begin("Maple Engine", &show_app_about, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return Result;
    }
    
    if (ImGui::BeginTabBar("Maple Engine Ui", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Engine Settings"))
        {
            
            if (ImGui::Button("Open Demo Window"))
            {
                AddWindowToStack(WindowStack, 
                                 &MapleDemoWindow, 
                                 &MapleDemoWindowFree,
                                 NULL);
            }
            
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Terrain Settings"))
        {
            TerrainUiTab(&Ui->TerrainSettings);
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Other Settings"))
        {
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
    
    return Result;
}

void MapleEngineDrawUi(window_stack *WindowStack)
{
    // Draw dev gui
    MapleDevGuiNewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    {
#if 0
        ImGui::ShowDemoWindow();
#else
        
        for (u32 i = 0; i < WindowStack->WindowCount; ++i)
        {
            ui_window Window = WindowStack->Stack[i];
            
            if (!Window.Callback(WindowStack, Window.Data))
            { // false is returned when the window was closed.
                WindowStack->Stack[i] = WindowStack->Stack[WindowStack->WindowCount - 1];
                WindowStack->WindowCount--;
            }
        }
#endif
    }
    
    ImGui::Render();
    ImDrawData* ImDrawData = ImGui::GetDrawData();
    MapleDevGuiRenderDrawData(ImDrawData);
    ImGui::EndFrame();
}

UI_WINDOW_DATA_FREE_CALLBACK(EngineMainUiFree)
{
}


UI_WINDOW_CALLBACK(MapleDemoWindow)
{
    bool Result = true;
    
    static bool show_app_about = false;
    if (!ImGui::Begin("Maple Demo Window", &show_app_about, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return Result;
    }
    
    if (ImGui::Button("Close"))
    {
        Result = false;
    }
    
    ImGui::End();
    
    return Result;
}

UI_WINDOW_DATA_FREE_CALLBACK(MapleDemoWindowFree)
{
}
