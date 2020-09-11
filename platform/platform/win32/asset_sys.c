
#define MOUNT_ROOT_IDX 0

typedef struct assetsys_file
{
    assetsys_file_type FileType;
    HANDLE           Handle;
} assetsys_file;

file_internal assetsys_error assetsys_translate_win32_error(u32 ErrorCode);
file_internal void assetsys_get_err_str(char *Buffer, u32 BufferLen, assetsys_error Error);

// Mount point routines
void assetsys_mount_point_init(assetsys_mount_point *MountPoint, assetsys_mount_type Type, 
                               const char *MountName, u32 NameLen,
                               asset_file_t File);
void assetsys_mount_point_free(assetsys_mount_point *MountPoint);

file_internal assetsys_error assetsys_translate_win32_error(u32 ErrorCode)
{
    assetsys_error Result = AssetSysErr_Count;
    
    if      (ErrorCode == ERROR_FILE_NOT_FOUND)      Result = AssetSysErr_FileNotFound;
    else if (ErrorCode == ERROR_PATH_NOT_FOUND)      Result = AssetSysErr_PathNotFound;
    else if (ErrorCode == ERROR_TOO_MANY_OPEN_FILES) Result = AssetSysErr_TooManyOpenFiles;
    else if (ErrorCode == ERROR_ACCESS_DENIED)       Result = AssetSysErr_AccessDenied;
    else if (ErrorCode == ERROR_INVALID_HANDLE)      Result = AssetSysErr_InvalidHandle;
    
    return Result;
}

file_internal void assetsys_get_err_str(char *Buffer, u32 BufferLen, assetsys_error Error)
{
    if (Error == AssetSysErr_FileNotFound) 
    {
        const char *ErrorMsg = "Asset System Error: File Not Found";
        u32 ErrorMsgLen = strlen(ErrorMsg);
        strncpy_s(Buffer, BufferLen, ErrorMsg, ErrorMsgLen);
    }
    
    else if (Error == AssetSysErr_PathNotFound) 
    {
        const char *ErrorMsg = "Asset System Error: Path Not Found";
        u32 ErrorMsgLen = strlen(ErrorMsg);
        strncpy_s(Buffer, BufferLen, ErrorMsg, ErrorMsgLen);
    }
    
    else if (Error == AssetSysErr_TooManyOpenFiles) 
    {
        const char *ErrorMsg = "Asset System Error: Too Many Open Files";
        u32 ErrorMsgLen = strlen(ErrorMsg);
        strncpy_s(Buffer, BufferLen, ErrorMsg, ErrorMsgLen);
    }
    
    else if (Error == AssetSysErr_AccessDenied) 
    {
        const char *ErrorMsg = "Asset System Error: Access Denied";
        u32 ErrorMsgLen = strlen(ErrorMsg);
        strncpy_s(Buffer, BufferLen, ErrorMsg, ErrorMsgLen);
    }
    
    else if (Error == AssetSysErr_InvalidHandle) 
    {
        const char *ErrorMsg = "Asset System Error: Invalid Handle";
        u32 ErrorMsgLen = strlen(ErrorMsg);
        strncpy_s(Buffer, BufferLen, ErrorMsg, ErrorMsgLen);
    }
    
}

file_internal void assetsys_internal_traverse_tree(const char *File, u32 Depth)
{
    WIN32_FIND_DATA FindFileData;
    char Path[2048];
    
    snprintf(Path, 2048, "%s/*", File);
    HANDLE Handle = FindFirstFileEx(Path, FindExInfoStandard, &FindFileData,
                                    FindExSearchNameMatch, NULL, 0);
    
    if (Handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (strcmp(FindFileData.cFileName, ".") != 0 &&
                strcmp(FindFileData.cFileName, "..") != 0 &&
                FindFileData.cFileName[0] != '.' ) // don't allow hidden files or folders
            {
                for (u32 i = 0; i < Depth; ++i) mprint("\t");
                
                if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    mprint(TEXT("%s   <DIR>\n"), FindFileData.cFileName);
                    
                    snprintf(Path, 2048, "%s/%s", File, FindFileData.cFileName);
                    assetsys_internal_traverse_tree(Path, Depth + 1);
                }
                else
                {
                    mprint(TEXT("%s\n"), FindFileData.cFileName);
                }
            }
        }
        while (FindNextFile(Handle, &FindFileData) != 0);
    }
}

void assetsys_init(assetsys *AssetSys, char *Root)
{
    if (!Root)
    {
        AssetSys->RootStr = Win32GetExeFilepath();
        // TODO(Dustin): Hash the root
    }
    else
    {
        // Verify the root directory exists
        BY_HANDLE_FILE_INFORMATION FileInfo;
        BOOL Err = GetFileAttributesEx(Root,
                                       GetFileExInfoStandard,
                                       &FileInfo);
        
        if (!Err)
        {
            u32 Win32Error = GetLastError();
            assetsys_error SysError = assetsys_translate_win32_error(Win32Error);
            
            char Msg[256];
            assetsys_get_err_str(Msg, 256, SysError);
            mprinte("Error initializing asset system: could not get root information.\n\t%s\n", Msg);
            return;
        }
        
        AssetSys->RootStr = mstr_init(Root, strlen(Root));
    }
    
    AssetSys->Root = hash_bytes(mstr_to_cstr(&AssetSys->RootStr), AssetSys->RootStr.Len);
    
    // TODO(Dustin): Create directory tree  
    
    // First, let's traverse the directory tree
    assetsys_internal_traverse_tree(mstr_to_cstr(&AssetSys->RootStr), 0);
    
    AssetSys->MountedFilesCap   = 10;
    AssetSys->MountedFilesCount = 0;
    AssetSys->MountedFiles = memory_alloc(Core->Memory, 
                                          AssetSys->MountedFilesCap * sizeof(assetsys_mount_point));
    
    // Mount the root directory
    assetsys_mount(AssetSys, mstr_to_cstr(&AssetSys->RootStr), false);
}


void assetsys_free(assetsys *AssetSys)
{
    mstr_free(&AssetSys->RootStr);
}

void assetsys_mount_point_init(assetsys_mount_point *MountPoint, assetsys_mount_type Type, 
                               const char *MountName, u32 NameLen,
                               asset_file_t File)
{
    MountPoint->Type = Type;
    MountPoint->Name = hash_bytes((void*)MountName, NameLen);
    MountPoint->File = File; // NOTE(Dustin): What if the file is NULL?
}

void assetsys_mount_point_free(assetsys_mount_point *MountPoint)
{
    
}

// Mount a file (file/directory/zip) from a name
void assetsys_mount(assetsys *AssetSys, const char *FileName, bool IsRelative)
{
    
}

// Mount a file (file/directory/zip) from a file pointer
void assetsys_mountf(assetsys *AssetSys, file_t File)
{
    
}

void assetsys_file_init(assetsys_file *File, const char *Filepath, u32 FilepathLen, 
                        bool IsRelative, const char *MountName, u32 MoutnNameLen)
{
    // If the filepath is a relative path, need to build the fullpath based on the
    // mountname - if one was provided
    if (IsRelative)
    {
        if (MountName)
        {
            // Search through known Mount Points to build the full path
        }
        else
        {
            // Build directly from the root
        }
    }
    else
    {
        // Absolute path was provided - can go ahead with the file creation process
        
    }
    
    // Will need this for getting the file type. 
    BY_HANDLE_FILE_INFORMATION FileInfo;
    //BOOL Err = GetFileAttributesEx(Root,
    //GetFileExInfoStandard,
    //&FileInfo);
    
}


void assetsys_file_free(assetsys_file *File)
{
    
}
