#ifndef ENGINE_UI_MAPLE_UI_SETTINGS_H
#define ENGINE_UI_MAPLE_UI_SETTINGS_H

struct terrain_settings
{
    bool    TerrainGenerated; // initially set to false, or maybe check if asset is valid?
    bool    TerrainMeshUpdated;
    bool    HeightmapUpdated;
    
    // Terrain Settings
    u32     TerrainWidth;
    u32     TerrainHeight;
    
    // Simple Sim. Settings
    u32     HeightmapWidth;
    u32     HeightmapHeight;
    u32     NumberOfOctaves;
    r32     Persistence;
    r32     Low;
    r32     High;
    r32     Exp;
    
    // Thermal Erosion Settings
    bool    ThermalEnabled;
    u32     ThermalNumIterations;
    
    // Inverse Thermal Settings
    bool    InverseThermalEnabled;
    u32     InverseThermalNumIterations;
    
    // Hydraulic Settings
    bool    HydraulicEnabled;
    u32     HydraulicNumIterations;
    r32     RainConstant;
    r32     SolubilityConstant;
    r32     EvaporationCoefficient;
    r32     SedimentTransferMaxCoefficient;
    
    mstring SaveFilename; // TODO(Dustin): Is this necessary anymore
};

struct maple_ui
{
    terrain_settings TerrainSettings;
    
};


#endif //ENGINE_UI_MAPLE_UI_SETTINGS_H
