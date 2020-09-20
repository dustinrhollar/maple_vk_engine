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

typedef enum file_error
{
    File_Success,
    File_FileNotFound,
    File_BufferTooSmall,
    File_UnableToRead,
} file_error;

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

typedef enum file_mode
{
    FileMode_Read,
    FileMode_Write,
    FileMode_ReadWrite,
    FileMode_Append,
} file_mode;

typedef u32                   file_id;
typedef struct assetsys_file* assetsys_file_t;
typedef struct assetsys*      assetsys_t;

#define assetsys_file_id_invalid (assetsys_file_id) { .Mask = 0 }
#define file_id_invalid 4294967295

#define file_id_is_valid(id) id != file_id_invalid
#define assetsys_valid_file_id(id) (id.Minor != 0) 

//~ Exposed assetsys api

void assetsys_init(assetsys_t AssetSys, char *Root);
void assetsys_free(assetsys_t AssetSys);

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
file_error file_load(const char *Filepath, bool IsRelative, const char *MountName,
                     void *Buffer, u64 Size);
void file_close(file_id Fid);

// Gets the size of a file that has been opened
u64 file_get_size(file_id Fid);
// Get size of a file without having to open it.
u64 file_get_fsize(const char *Filename, const char *MountName);

//~ Asset API

void load_asset(const char *Filename, asset_type Type);

#endif //PLATFORM_ASSET_SYS_H
