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
    FileType_Unknown = FileType_Count, // used for checking if file is initialized
} assetsys_file_type;

typedef enum assetsys_mount_type
{
    MountType_Directory,
    MountType_Zip, // NOTE(Dustin): Not implemented, and won't be for a while
    
    MountType_Count,
    MountType_Unknown = MountType_Count, 
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

// A file id is a bitmask that can locate the file within the
// assetsys_file_pool data structure. An assetsys instance will
// contain *at most* 256 assetsys_file_pools, each with the capacity
// of 256 files. This allows for up to 65,536 files for a single
// instance assetsys. 
//
// A file_id is a bit mask for the major and minor pool index.
// A major pool index is the index in the assetsys_file_pool array
// whereas the minor pool index is the index into the file array
// inside a pool. The lower 8 bits of the file id are the major 
// pool index and the upper 8 bits are the minor pool index.
//
// Example:
//
// file_id Fid;
// ...
// assetsys_file_pool *Pool = AsseySys->FilePool + Fid.Major;
// asset_file_t File = Pool->Handles + Fid.Minor;
//
typedef union file_id
{
    struct { u8 Major, Minor; };
    u16 Mask;
} file_id;

#define file_id_invalid { .Major = 0, .Minor = 0 }

typedef struct assetsys_file
{
    file_id            Id; // backpointer to the file array
    assetsys_file_type Type;
    HANDLE             Handle;
    
    // A directory can have 0 or more files.
    // ".", "..", and hidden files/directories are ignored
    file_id           *ChildFiles;
    u32                ChildFileCount;
    
    // File info
    mstr Name;
    u128 HashedName;
    
} assetsys_file;

typedef struct assetsys_file* asset_file_t;

typedef struct assetsys_mount_point
{
    assetsys_mount_type Type;
    u128                Name;
    file_id             File; // backpointer to the zip/directory
} assetsys_mount_point;

#define MAX_ASSETSYS_POOL_COUNT      256
#define MAX_ASSETSYS_POOL_FILE_COUNT 256
typedef struct assetsys_file_pool
{
    assetsys_file *Handles;
    u8             AllocatedFiles;
} assetsys_file_pool;

void assetsys_file_pool_init(assetsys_file_pool *FilePool);
void assetsys_file_pool_free(assetsys_file_pool *FilePool);
// If the pool is full (Pool == NULL), then File will remain NULL.
void assetsys_file_pool_alloc(assetsys_file_pool *FilePool, asset_file_t *File);
// the reason why the release function takes a Fid and not the alloc function, is
// because only the function that calls alloc should have the pointer. When the function
// exits, that pointer should become stale. For example:
//
// file_id foo(assetsys_file_pool *FilePool) {
//     asset_file_t File = NULL;
//     assetsys_file_pool_alloc(FilePool, &File);
//
//     // do some stuff with File
//
//     // if someone needs the file, return the file id, not the file pointer!
//     return File->Id;
// }
//
// Since it is expected that the caller only has access to the file_id,
// the release function should take the file_id rather than the file pointer.
void assetsys_file_pool_release(assetsys_file_pool *FilePool, file_id Fid);

typedef struct assetsys
{
    mstr   RootStr;
    u128   Root;
    
    // By default, the root is a mounted file at idx = 0
    u32                   MountedFilesCount;
    u32                   MountedFilesCap;
    assetsys_mount_point *MountedFiles;
    
    // File pool for file allocations
    u32                 FilePoolCount;
    u32                 FilePoolCap;
    assetsys_file_pool *FilePool;
    
} assetsys;

//~ AssetSys Api

// If Root == NULL, then Win32GetExeFilepath is called
void assetsys_init(assetsys *AssetSys, char *Root);
void assetsys_free(assetsys *AssetSys);

// Mount a file (file/directory/zip) from a name
void assetsys_mount(assetsys *AssetSys, const char *FileName, const char *MountName, bool IsRelative);
// Mount a file (file/directory/zip) from a file pointer
// TODO(Dustin): Change to be from a file_id
void assetsys_mountf(assetsys *AssetSys, file_t File);

file_t assetsys_open(assetsys *AssetSys, const char *FileName, bool IsRelative);
file_t assetsys_load(assetsys *AssetSys, const char *FileName, bool IsRelative);
file_t assetsys_close(assetsys *AssetSys, file_t File);

//~ File Api

void file_print_directory_tree(const char *MountName);

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
