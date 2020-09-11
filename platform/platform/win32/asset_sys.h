/* date = September 9th 2020 9:03 pm */

#ifndef PLATFORM_ASSET_SYS_H
#define PLATFORM_ASSET_SYS_H

#if 0
typedef enum assetsys_type
{
    // define things like: scene, material, model, etc?
} assetsys_type;
#endif

typedef enum assetsys_file_type
{
    FileType_Volume = 0,
    FileType_Directory,
    FileType_File,
    
    FileType_Count,
} assetsys_file_type;

typedef enum assetsys_mount_type
{
    MountType_Directory,
    MountType_Zip, // NOTE(Dustin): Not implemented, and won't be for a while
    
    MountType_Count,
} assetsys_mount_type;

typedef enum assetsys_error
{
    // Asset Erro Name            // Win32 Error Code #
    AssetSysErr_FileNotFound = 0, // 2
    AssetSysErr_PathNotFound,     // 3
    AssetSysErr_TooManyOpenFiles, // 4
    AssetSysErr_AccessDenied,     // 5 
    AssetSysErr_InvalidHandle,    // 6
    
    // TODO(Dustin): Finish wrapping the error messages
    // Want to make the header file platform independent. 
    // A particular platform can then implement the functionality.
    // https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes--0-499-
    
    AssetSysErr_Count,
} assetsys_error;

typedef struct assetsys_file* asset_file_t;

typedef struct assetsys_mount_point
{
    assetsys_mount_type Type;
    u128                Name;
    asset_file_t        File; // backpointer to the zip/directory
} assetsys_mount_point;

typedef struct assetsys
{
    mstr   RootStr;
    u128   Root;
    
    // By default, the root is a mounted file at idx = 0
    u32                   MountedFilesCount;
    u32                   MountedFilesCap;
    assetsys_mount_point *MountedFiles;
} assetsys;

//~ AssetSys Api

// If Root == NULL, then Win32GetExeFilepath is called
void assetsys_init(assetsys *AssetSys, char *Root);
void assetsys_free(assetsys *AssetSys);

// Mount a file (file/directory/zip) from a name
void assetsys_mount(assetsys *AssetSys, const char *FileName, bool IsRelative);
// Mount a file (file/directory/zip) from a file pointer
void assetsys_mountf(assetsys *AssetSys, file_t File);

file_t assetsys_open(assetsys *AssetSys, const char *FileName, bool IsRelative);
file_t assetsys_load(assetsys *AssetSys, const char *FileName, bool IsRelative);
file_t assetsys_close(assetsys *AssetSys, file_t File);

//~ File Api

// These are mostly wrappers around the asset system's file related api.
// That way, the platform can simply expose functions like:
// file_open(...), file_close(...)
// without expecting externals like the graphics and game modules to know
// about the asset system.

// open
// load
// close
// binary write
// formatted write

#endif //PLATFORM_ASSET_SYS_H
