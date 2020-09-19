#ifndef PLATFORM_ASSET_SYS_H
#define PLATFORM_ASSET_SYS_H

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
typedef union assetsys_file_id
{
    struct { u8 Major, Minor; };
    u16 Mask;
} assetsys_file_id;

#define assetsys_file_id_invalid (assetsys_file_id) { .Mask = 0 }
#define file_id_invalid 4294967295

#define file_id_is_valid(id) id != file_id_invalid
#define assetsys_valid_file_id(id) (id.Minor != 0) 

/*

Read Info
- HANDLE
- 4kb buffer
- Current Offset into buffer

Write
- HANDLE                              
- 4kb buffer                          void* -> Allocated App Memory or VirtualAlloc'd ?
- Current Offset into buffer          u32
- Current Offset into the file buffer u64


Create an array of 4kb alllocations - set initalize 10 block 4kb
// These are all Virtual Alloc'd

VirtualAlloc()

....

VirtualFree()

*/

// 8kb -> needs to writes

typedef enum file_mode
{
    FileMode_Read,
    FileMode_Write,
    FileMode_ReadWrite,
    FileMode_Append,
} file_mode;

typedef struct file_info
{
    file_mode        Mode;
    HANDLE           Handle;
    u64              Size;
    
    void            *Memory;
    u32              MemoryOffset;
    
    u64              FileOffset;
    
    assetsys_file_id Fid;
} file_info;
typedef u32 file_id;

typedef struct assetsys_file* assetsys_file_t;

typedef struct assetsys_mount_point
{
    assetsys_mount_type Type;
    u128                Name;
    
    mstr                AbsolutePath;
    assetsys_file_id    File; // backpointer to the zip/directory
} assetsys_mount_point;

#define MAX_ASSETSYS_POOL_COUNT      256
#define MAX_ASSETSYS_POOL_FILE_COUNT 256
typedef struct assetsys_file_pool
{
    assetsys_file_t Handles;
    u8              AllocatedFiles;
} assetsys_file_pool;

void assetsys_file_pool_init(assetsys_file_pool *FilePool);
void assetsys_file_pool_free(assetsys_file_pool *FilePool);
// If the pool is full (Pool == NULL), then File will remain NULL.
void assetsys_file_pool_alloc(assetsys_file_pool *FilePool, assetsys_file_t *File);
// the reason why the release function takes a Fid and not the alloc function, is
// because only the function that calls alloc should have the pointer. When the function
// exits, that pointer should become stale. For example:
//
// assetsys_file_id foo(assetsys_file_pool *FilePool) {
//     asset_file_t File = NULL;
//     assetsys_file_pool_alloc(FilePool, &File);
//
//     // do some stuff with File
//
//     // if someone needs the file, return the file id, not the file pointer!
//     return File->Id;
// }
//
// Since it is expected that the caller only has access to the assetsys_file_id,
// the release function should take the assetsys_file_id rather than the file pointer.
void assetsys_file_pool_release(assetsys_file_pool *FilePool, assetsys_file_id Fid);

static const u32 MAX_OPEN_FILES = 128;
typedef struct assetsys
{
    mstr   RootStr;
    u128   Root;
    
    // By default, the root is a mounted file at idx = 0
    u32                   MountedFilesCount;
    u32                   MountedFilesCap;
    assetsys_mount_point *MountedFiles;
    
    // File pool for file allocations
    u32                   FilePoolCount;
    u32                   FilePoolCap;
    assetsys_file_pool   *FilePool;
    
    // Track open files...
    void*                 FileMemory[MAX_OPEN_FILES];
    file_info             OpenFiles[MAX_OPEN_FILES];
    u64                   OpenFilesMask[2];
    
} assetsys;

//~ AssetSys Api

// If Root == NULL, then Win32GetExeFilepath is called
void assetsys_init(assetsys *AssetSys, char *Root);
void assetsys_free(assetsys *AssetSys);

// Mount a file (file/directory/zip) from a name
void assetsys_mount(assetsys *AssetSys, const char *FileName, const char *MountName);
// Mount a file (file/directory/zip) relative to another mount 
void assetsys_mountr(assetsys *AssetSys, const char *FileName, const char *MountName, const char *RelativeMountName);

file_id assetsys_open(assetsys *AssetSys, const char *Filepath, bool IsRelative, const char *MountName, file_mode Mode);
file_id assetsys_load(assetsys *AssetSys, const char *Filepath, bool IsRelative, const char *MountName);
void assetsys_close(assetsys *AssetSys, file_id File);

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

file_id file_open(const char *Filepath, bool IsRelative, const char *MountName, file_mode Mode);
file_id file_load(const char *Filepath, bool IsRelative, const char *MountName);
void file_close(file_id Fid);

//~ Asset API

// An asset is anything that can be loaded from a file
// the asset type dictates where the asset is located on disc.
// For example, a shader asset will be located in the data\shaders
// directory.
typedef enum asset_type
{
    Asset_Model,
    Asset_Image,
    Asset_Shader,
    Asset_PipelineCache,
    
    Asset_Count,
} asset_type;

void load_asset(const char *Filename, asset_type Type);

#endif //PLATFORM_ASSET_SYS_H
