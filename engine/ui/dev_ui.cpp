
struct asset;

DEV_UI_DRAW_CALLBACK(MapleDevUi)
{
    ImGui::NewFrame();
    
#if 0
    // Render the demo window
    ImGui::ShowDemoWindow();
    
#else
    
    masset::asset_registry *AssetRegistry = masset::GetAssetRegistry();
    
    ImGui::SetNextWindowSize(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    
    ImGuiWindowFlags window_flags = 0;
	if (!ImGui::Begin("Maple Editor UI", nullptr, window_flags)) // No bool flag to omit the close button
	{
		// Early out if the window is collapsed, as an optimization
		ImGui::End();
		return;
	}
    
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f); // right alifned, keep 180 pixels for the labels
    
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("World Settings", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Game Settings"))
        {
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Engine Settings"))
        {
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Models"))
        {
            // TODO(Dustin): To start off with, only show local coordinates of a mesh
            // In the future, will need to traverse the node heirarchy to get the
            // appropriate model matrix and extract the position, scale, and rotation
            // of the mesh.
            
            // TODO(Dustin): Allow for mesh data to be edited
            
            for (u32 ModelIdx = 0; ModelIdx < AssetRegistry->ModelAssets.Size; ++ModelIdx)
            {
                asset_id_t AssetId = AssetRegistry->ModelAssets.Ptr[ModelIdx];
                asset_model *Model = &AssetRegistry->Assets[AssetId.Index]->Model;
                
                for (u32 MeshIdx = 0; MeshIdx < Model->MeshesCount; ++MeshIdx)
                {
                    mesh *Mesh = &Model->Meshes[MeshIdx];
                    
                    if (ImGui::CollapsingHeader(Mesh->Name.GetCStr()))
                    {
                    }
                }
            }
            
            
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Materials"))
        {
            asset *MaterialList = nullptr;
            u32 MaterialListCount = 0;
            masset::FilterAssets(&MaterialList, &MaterialListCount, nullptr, 0, Asset_Material);
            
            for (u32 AssetIdx = 0; AssetIdx < AssetRegistry->Count; ++AssetIdx)
            {
                if (AssetRegistry->Assets[AssetIdx]->Type == Asset_Material)
                {
                    asset_material *Material = &AssetRegistry->Assets[AssetIdx]->Material;
                    
                    if (ImGui::CollapsingHeader(Material->Name.GetCStr()))
                    {
                        material_instance *Instance = &Material->Instance;
                        
                        if (Instance->HasPBRMetallicRoughness)
                        {
                            if (ImGui::InputFloat4("Base Color Factor", Instance->BaseColorFactor.data))
                            {
                            }
                            
                            if (ImGui::InputFloat("Metallic Factor", &Instance->MetallicFactor))
                            {
                            }
                            
                            if (ImGui::InputFloat("Roughness Factor", &Instance->RoughnessFactor))
                            {
                            }
                            
                        }
                        
                        if (Instance->HasPBRSpecularGlossiness)
                        {
                            if (ImGui::InputFloat4("Diffuse Factor", Instance->DiffuseFactor.data))
                            {
                            }
                            
                            if (ImGui::InputFloat3("Specular Factor", Instance->SpecularFactor.data))
                            {
                            }
                            
                            if (ImGui::InputFloat("Glossiness Factor", &Instance->GlossinessFactor))
                            {
                            }
                        }
                        
                        if (Instance->HasClearCoat)
                        {
                            if (ImGui::InputFloat("Clear Coat Factor", &Instance->ClearCoatFactor))
                            {
                            }
                            
                            if (ImGui::InputFloat("Clear Coat Roughness Factor", &Instance->ClearCoatRoughnessFactor))
                            {
                            }
                        }
                    }
                }
            }
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    ImGui::Separator();
    
    ImGui::PopItemWidth();
	ImGui::End();
#endif
    
    // Render to generate draw buffers
    ImGui::Render();
    ImDrawData* ImDrawData = ImGui::GetDrawData();
    MapleDevGuiDraw(DevGui, CommandBuffer);
    ImGui::EndFrame();
}