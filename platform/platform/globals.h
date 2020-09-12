#ifndef ENGINE_PLATFORM_GLOBALS_H
#define ENGINE_PLATFORM_GLOBALS_H

typedef struct 
{
    u64 Size;
} memory_create_info;

typedef struct
{
    const char *Path;       // path to the mount
    const char *MountName;  // Desired name of the mount 
    bool        IsRelative; // if the mount is relative to the .exe
} assetsys_mount_point_create_info;

typedef struct 
{
    // Allowed to be null. If null, the Win32GetExePath is used
    // by the asset system
    const char                       *ExecutablePath;
    
    assetsys_mount_point_create_info *MountPoints;
    u32                               MountPointsCount;
    
} assetsys_create_info;

typedef struct 
{
    memory_create_info   Memory;
    assetsys_create_info AssetSystem;
} globals_create_info;

typedef struct
{
    struct memory   *Memory;
    struct assetsys *AssetSys;
} globals;

extern globals *Core;

void globals_init(globals_create_info *CreateInfo);
void globals_free();

#endif //GLOBALS_H
