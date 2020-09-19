
typedef struct assetsys_file
{
    assetsys_file_id   Id; // backpointer to the file array
    assetsys_file_type Type;
    
    file_id            FileInfo;
    
    // A directory can have 0 or more files.
    // ".", "..", and hidden files/directories are ignored
    assetsys_file_id  *ChildFiles;
    u32                ChildFileCount;
    
    // File info
    mstr Name;
    u128 HashedName;
    
} assetsys_file;

#define MOUNT_ROOT_IDX 0
// Minor Idx = 0 is the invalid case, so always subtract by one when indexing
#define assetsys_get_file(sys, id) sys->FilePool[id.Major].Handles + (id.Minor - 1)

// When looking for a file id, it will be common that the 
// file is nested within several directories. However, the absolute paths
// of the directories are not stored. Therefore, preprocess the filepath
// into a list of comparators and search for the directories.
typedef struct comparator_list
{
    u32   Count;
    u32   Idx;
    u128 *Comparators;
} comparator_list;

// Error functions
file_internal assetsys_error assetsys_translate_win32_error(u32 ErrorCode);
file_internal void assetsys_get_err_str(char *Buffer, u32 BufferLen, assetsys_error Error);

// tree traversals
file_internal void assetsys_build_comparator_list(comparator_list *List, const char *Filepath);

file_internal void assetsys_internal_traverse_tree(assetsys *AssetSys, assetsys_file_id Fid, u32 Depth);
file_internal assetsys_file_id assetsys_find_fid(assetsys *AssetSys, assetsys_file_id CurrentId, comparator_list *CompList);
file_internal assetsys_mount_point assetsys_find_mount_point(assetsys *AssetSys, const char *MountName);
file_internal void assetsys_file_init_recurse_directory(assetsys *AssetSys, assetsys_file_id ParentFid, const char *Filepath);
file_internal assetsys_file_id assetsys_insert_file_in_tree(assetsys *AssetSys, 
                                                            comparator_list *CompList, 
                                                            assetsys_file_id MountFid, 
                                                            const char *Filename, u32 FilenameLen,
                                                            const char *Directory, u32 DirectoryLen);

// File
file_internal assetsys_file_id assetsys_allocate_file(assetsys *AssetSys, assetsys_file_type FileType);
file_internal assetsys_file_id assetsys_file_init(assetsys *AssetSys, const char *Filename, u32 FilenameLen, 
                                                  bool IsRelative, const char *DirPath, u32 DirPathLen);
file_internal void assetsys_file_free(assetsys_file *File);

// File Pool
file_internal void assetsys_file_pool_init(assetsys_file_pool *FilePool);
file_internal void assetsys_file_pool_free(assetsys_file_pool *FilePool);
file_internal void assetsys_file_pool_alloc(assetsys_file_pool *FilePool, assetsys_file **File);
file_internal void assetsys_file_pool_release(assetsys_file_pool *FilePool, assetsys_file_id Fid);

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

file_internal void assetsys_internal_traverse_tree(assetsys *AssetSys, assetsys_file_id Fid, u32 Depth)
{
    assetsys_file *File = assetsys_get_file(AssetSys, Fid);
    const char *Filepath = mstr_to_cstr(&File->Name);
    
    for (u32 i = 0; i < Depth; ++i) mprint("\t");
    mprint("%s\n", Filepath);
    
    if (File->Type == FileType_Directory)
    {
        for (u32 i = 0; i < File->ChildFileCount; ++i) 
            assetsys_internal_traverse_tree(AssetSys, File->ChildFiles[i], Depth + 1);
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
    
    // Setup the file_pool major list
    AssetSys->FilePoolCount = 1;
    AssetSys->FilePoolCap   = 1;
    AssetSys->FilePool = (assetsys_file_pool*)memory_alloc(Core->Memory, 
                                                           sizeof(assetsys_file_pool) * AssetSys->FilePoolCap);
    assetsys_file_pool_init(AssetSys->FilePool + 0);
    
    // Setup the Mounted files list
    AssetSys->MountedFilesCap   = 10;
    AssetSys->MountedFilesCount = 0;
    AssetSys->MountedFiles = memory_alloc(Core->Memory, 
                                          AssetSys->MountedFilesCap * sizeof(assetsys_mount_point));
    
    for (u32 i = 0; i < AssetSys->MountedFilesCap; ++i)
    {
        AssetSys->MountedFiles[i].Type = MountType_Unknown;
    }
    
    AssetSys->OpenFilesMask[0] = 0;
    AssetSys->OpenFilesMask[1] = 0;
    
    for (u32 i = 0; i < MAX_OPEN_FILES; ++i)
    {
        AssetSys->OpenFiles[i].Handle       = INVALID_HANDLE_VALUE;
        AssetSys->OpenFiles[i].Size         = 0;
        AssetSys->OpenFiles[i].Memory       = NULL;
        AssetSys->OpenFiles[i].MemoryOffset = 0;
        AssetSys->OpenFiles[i].FileOffset   = 0;
        AssetSys->OpenFiles[i].Fid          = assetsys_file_id_invalid;
    }
    
    // Mount the root directory, will initialize the file tree for the root directory
    //assetsys_mount(AssetSys, mstr_to_cstr(&AssetSys->RootStr), "root", false);
}

void assetsys_free(assetsys *AssetSys)
{
    mstr_free(&AssetSys->RootStr);
    
    if (AssetSys->MountedFiles) memory_release(Core->Memory, AssetSys->MountedFiles);
    AssetSys->MountedFilesCap = 0;
    AssetSys->MountedFilesCount = 0;
    
    for (u32 i = 0; i < AssetSys->FilePoolCount; ++i)
    {
        assetsys_file_pool_free(AssetSys->FilePool + i);
    }
    
    if (AssetSys->FilePool) memory_release(Core->Memory, AssetSys->FilePool);
    AssetSys->FilePoolCap = 0;
    AssetSys->FilePoolCount = 0;
}

file_internal assetsys_file_id assetsys_find_fid(assetsys *AssetSys, assetsys_file_id CurrentId, comparator_list *CompList)
{
    assetsys_file_id Result = assetsys_file_id_invalid;
    
    assetsys_file *File = assetsys_get_file(AssetSys, CurrentId);
    u128 Comparator = CompList->Comparators[CompList->Idx];
    
    for (u32 i = 0; i < File->ChildFileCount; ++i)
    {
        assetsys_file *ChildFile = assetsys_get_file(AssetSys, File->ChildFiles[i]);
        
        if (compare_hash(ChildFile->HashedName, Comparator))
        {
            if (CompList->Idx + 1 == CompList->Count)
            {
                Result = ChildFile->Id;
            }
            else
            {
                CompList->Idx++;
                Result = assetsys_find_fid(AssetSys, File->ChildFiles[i], CompList);
            }
            
            break;
        }
    }
    
    return Result;
}


file_internal assetsys_file_id assetsys_insert_file_in_tree(assetsys *AssetSys, 
                                                            comparator_list *CompList, 
                                                            assetsys_file_id MountFid, 
                                                            const char *Filename, u32 FilenameLen,
                                                            const char *Directory, u32 DirectoryLen)
{
    assetsys_file_id Result = assetsys_file_id_invalid;
    
    assetsys_file *File = assetsys_get_file(AssetSys, MountFid);
    u128 Comparator = CompList->Comparators[CompList->Idx];
    
    for (u32 i = 0; i < File->ChildFileCount; ++i)
    {
        assetsys_file *ChildFile = assetsys_get_file(AssetSys, File->ChildFiles[i]);
        
        if (compare_hash(ChildFile->HashedName, Comparator))
        {
            if (CompList->Idx + 1 == CompList->Count)
            {
                // reached the end of the comp list, time to quit
                break;
            }
            
            CompList->Idx++;
            File = ChildFile;
            i = 0;
        }
    }
    
    Result = assetsys_file_init(AssetSys, Filename, FilenameLen, true, Directory, DirectoryLen);
    
    // Insert the new file into the asset list
    assetsys_file_id *ChildFilesCpy = memory_alloc(Core->Memory, sizeof(assetsys_file_id) * File->ChildFileCount + 1);
    memcpy(ChildFilesCpy, File->ChildFiles, sizeof(assetsys_file_id) * File->ChildFileCount);
    File->ChildFiles[File->ChildFileCount++] = Result;
    
    return Result;
}

file_internal assetsys_mount_point assetsys_find_mount_point(assetsys *AssetSys, const char *MountName)
{
    assetsys_mount_point Result;
    
    u128 MountHash = hash_bytes((void*)MountName, strlen(MountName));
    
    for (u32 i = 0; i < AssetSys->MountedFilesCount; ++i)
    {
        if (compare_hash(MountHash, AssetSys->MountedFiles[i].Name))
        {
            Result = AssetSys->MountedFiles[i];
            break;
        }
    }
    
    return Result;
}

// Mount a file (file/directory/zip) from a name
void assetsys_mount(assetsys *AssetSys, const char *Filename, const char *MountName)
{
    assetsys_file_id Root = assetsys_file_init(AssetSys, Filename, strlen(Filename), false, NULL, 0);
    
    if (AssetSys->MountedFilesCount + 1 > AssetSys->MountedFilesCap)
    {
        u32 NewCap = AssetSys->MountedFilesCap * 2;
        assetsys_mount_point *MountedFiles = memory_alloc(Core->Memory, sizeof(assetsys_mount_point) * NewCap);
        
        for (u32 i = 0; i < AssetSys->MountedFilesCount; ++i) MountedFiles[i] = AssetSys->MountedFiles[i];
        memory_release(Core->Memory, AssetSys->MountedFiles);
        
        AssetSys->MountedFilesCap = NewCap;
        AssetSys->MountedFiles = MountedFiles;
    }
    
    assetsys_mount_point Mount = {0};
    Mount.Type = MountType_Directory;
    Mount.Name = hash_bytes((void*)MountName, strlen(MountName));
    Mount.File = Root;
    Mount.AbsolutePath = mstr_init((char*)Filename, strlen(Filename));
    
    AssetSys->MountedFiles[AssetSys->MountedFilesCount++] = Mount;
}

void assetsys_mountr(assetsys *AssetSys, const char *Filename, const char *MountName, const char *RelativeMountName)
{
    assetsys_file_id ParentMountFid = assetsys_find_mount_point(AssetSys, RelativeMountName).File;
    
    if (!assetsys_valid_file_id(ParentMountFid))
    {
        mprinte("Unable to find the parent mount \"%s\"!\n", RelativeMountName);
        return;
    }
    
    // File is relative to the above mount point, files are already allocated.
    // Find the assetsys_file_id to mount it.
    u128 HashedMountName = hash_bytes((void*)MountName, strlen(MountName));
    
    u32 Count = 1;
    char *pch;
    
    // how many directories are there?
    pch = strchr(Filename, '/');
    
    while (pch != NULL)
    {
        pch = strchr(pch + 1, '/');
        Count++;
    };
    
    // Build the comparator list
    comparator_list CompList = {0};
    CompList.Count = Count;
    CompList.Idx = 0;
    CompList.Comparators = memory_alloc(Core->Memory, CompList.Count * sizeof(u128));
    
    pch = NULL;
    pch = strchr(Filename, '/');
    char *Offset = (char*)Filename;
    
    while (pch != NULL)
    {
        CompList.Comparators[CompList.Idx++] = hash_bytes(Offset, pch - Offset);
        Offset += pch - Offset + 1;
        pch = strchr(pch + 1, '/');
    }
    
    CompList.Comparators[CompList.Idx] = hash_bytes(Offset, strlen(Filename) - (Offset - Filename));
    CompList.Idx = 0;
    
    // Start the scan with the root.
    assetsys_file_id RootFid = AssetSys->MountedFiles[MOUNT_ROOT_IDX].File;
    assetsys_file_id MountFid = assetsys_find_fid(AssetSys, ParentMountFid, &CompList);
    
    // release comparator list
    if (CompList.Comparators) memory_release(Core->Memory, CompList.Comparators);
    
    if (assetsys_valid_file_id(MountFid))
    {
        if (AssetSys->MountedFilesCount + 1 > AssetSys->MountedFilesCap)
        {
            u32 NewCap = AssetSys->MountedFilesCap * 2;
            assetsys_mount_point *MountedFiles = memory_alloc(Core->Memory, sizeof(assetsys_mount_point) * NewCap);
            
            for (u32 i = 0; i < AssetSys->MountedFilesCount; ++i) MountedFiles[i] = AssetSys->MountedFiles[i];
            memory_release(Core->Memory, AssetSys->MountedFiles);
            
            AssetSys->MountedFilesCap = NewCap;
            AssetSys->MountedFiles = MountedFiles;
        }
        
        assetsys_mount_point Mount = {0};
        Mount.Type = MountType_Directory;
        Mount.Name = HashedMountName;
        Mount.File = MountFid;
        
        
        assetsys_file *ParentFile = assetsys_get_file(AssetSys, ParentMountFid);
        
        char Path[2048];
        int WrittenChars = snprintf(Path, 2048, "%s/%s", mstr_to_cstr(&ParentFile->Name), Filename);
        Mount.AbsolutePath = mstr_init(Path, WrittenChars);
        
        AssetSys->MountedFiles[AssetSys->MountedFilesCount++] = Mount;
    }
    else
    {
        mprinte("Unable to mount \"%s\"!\n", Filename);
    }
}

file_internal assetsys_file_id assetsys_allocate_file(assetsys *AssetSys, assetsys_file_type FileType)
{
    assetsys_file *File = NULL;
    bool Found = false;
    
    // Scan through the existing File Pools to allocate a file
    for (u32 i = 0; i < AssetSys->FilePoolCount; ++i)
    {
        assetsys_file_pool_alloc(AssetSys->FilePool + i, &File);
        
        if (File)
        {
            File->Id.Major       = i;
            File->Type           = FileType;
            File->FileInfo       = file_id_invalid;
            File->ChildFiles     = NULL;
            File->ChildFileCount = 0;
            
            Found = true;
            break;
        }
    }
    
    // If a File was not allocated from the existing Pools, create
    // a new Pool and allocate from that one.
    if (!Found)
    {
        if (AssetSys->FilePoolCount + 1 > AssetSys->FilePoolCap)
        {
            u32 NewCap = AssetSys->FilePoolCap;
            NewCap = (NewCap > MAX_ASSETSYS_POOL_COUNT) ? MAX_ASSETSYS_POOL_COUNT : NewCap; 
            assetsys_file_pool *FilePool = (assetsys_file_pool*)memory_alloc(Core->Memory, 
                                                                             sizeof(assetsys_file_pool) * NewCap);
            
            for (u32 i = 0; i < AssetSys->FilePoolCount; ++i) FilePool[i] = AssetSys->FilePool[i];
            
            memory_release(Core->Memory, AssetSys->FilePool);
            AssetSys->FilePool = FilePool;
        }
        
        assetsys_file_pool_init(AssetSys->FilePool + AssetSys->FilePoolCount);
        assetsys_file_pool_alloc(AssetSys->FilePool + AssetSys->FilePoolCount, &File);
        
        if (!File) 
            mprinte("Failed to allocate file memory. Probably ran out of it.\n");
        else 
        {
            File->Id.Major       = AssetSys->FilePoolCount;
            File->Type           = FileType;
            File->FileInfo       = file_id_invalid;
            File->ChildFiles     = NULL;
            File->ChildFileCount = 0;
            
            AssetSys->FilePoolCount++;
        }
    }
    
    return File->Id;
}

file_internal void assetsys_file_init_recurse_directory(assetsys *AssetSys, assetsys_file_id ParentFid, const char *Filepath)
{
    assetsys_file *ParentFile = assetsys_get_file(AssetSys, ParentFid);
    
    WIN32_FIND_DATA FindFileData;
    char Path[2048];
    
    snprintf(Path, 2048, "%s/*", Filepath);
    
    // Preprocess the directory file count
    HANDLE Handle = FindFirstFileEx(Path, FindExInfoStandard, &FindFileData,
                                    FindExSearchNameMatch, NULL, 0);
    
    u32 ChildCount = 0;
    if (Handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (strcmp(FindFileData.cFileName, ".") != 0 &&
                strcmp(FindFileData.cFileName, "..") != 0 &&
                FindFileData.cFileName[0] != '.' ) // don't allow hidden files or folders
            {
                ++ChildCount;
            }
        }
        while (FindNextFile(Handle, &FindFileData) != 0);
    }
    FindClose(Handle);
    
    ParentFile->ChildFileCount = ChildCount;
    ParentFile->ChildFiles = (assetsys_file_id*)memory_alloc(Core->Memory, sizeof(assetsys_file_id) * ParentFile->ChildFileCount);
    
    // Now, load the files
    ChildCount = 0;
    
    Handle = FindFirstFileEx(Path, FindExInfoStandard, &FindFileData,
                             FindExSearchNameMatch, NULL, 0);
    
    if (Handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (strcmp(FindFileData.cFileName, ".") != 0 &&
                strcmp(FindFileData.cFileName, "..") != 0 &&
                FindFileData.cFileName[0] != '.' ) // don't allow hidden files or folders
            {
                snprintf(Path, 2048, "%s/%s", Filepath, FindFileData.cFileName);
                
                assetsys_file_id ChildFid = assetsys_file_init(AssetSys, 
                                                               FindFileData.cFileName, strlen(FindFileData.cFileName), 
                                                               true, 
                                                               Filepath, strlen(Filepath));
                
                ParentFile->ChildFiles[ChildCount++] = ChildFid;
            }
        }
        while (FindNextFile(Handle, &FindFileData) != 0);
    }
    
    FindClose(Handle);
}

file_internal assetsys_file_id assetsys_file_init(assetsys *AssetSys, const char *Filename, u32 FilenameLen, 
                                                  bool IsRelative, const char *DirPath, u32 DirPathLen)
{
    assetsys_file_id Result = {0};
    
    BY_HANDLE_FILE_INFORMATION FileInfo;
    BOOL Err;
    
    // If the filepath is a relative path, need to build the fullpath based on the
    // mountname - if one was provided
    if (IsRelative)
    {
        char Path[2048];
        
        char *Iter = Path;
        memcpy(Iter, DirPath, DirPathLen);
        Iter += DirPathLen;
        
        if (DirPath)
        {
            snprintf(Iter, 2048, "/%s", Filename);
        }
        else
        {
            // Build directly from the root
            assetsys_mount_point *Mount = AssetSys->MountedFiles + 0;
            assetsys_file *File = assetsys_get_file(AssetSys, Mount->File);
            
            snprintf(Path, 2048, "%s/%s", 
                     mstr_to_cstr(&File->Name), 
                     Filename);
        }
        
        Err = GetFileAttributesEx(Path,
                                  GetFileExInfoStandard,
                                  &FileInfo);
        
        if (!Err) mprinte("Error getting file attribute when initializing file %s!\n", Filename);
        else
        {
            if (FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                Result = assetsys_allocate_file(AssetSys, FileType_Directory);
                
                assetsys_file *File = assetsys_get_file(AssetSys, Result);
                File->Name = mstr_init((char*)Filename, FilenameLen);
                File->HashedName = hash_bytes((void*)Filename, FilenameLen);
                
                assetsys_file_init_recurse_directory(AssetSys, Result, Path);
            }
            else
            {
                Result = assetsys_allocate_file(AssetSys, FileType_File);
                
                assetsys_file *File = assetsys_get_file(AssetSys, Result);
                File->Name = mstr_init((char*)Filename, FilenameLen);
                File->HashedName = hash_bytes((void*)Filename, FilenameLen);
            }
        }
    }
    else
    {
        // Absolute path was provided - can go ahead with the file creation process
        Err = GetFileAttributesEx(Filename,
                                  GetFileExInfoStandard,
                                  &FileInfo);
        
        if (!Err) mprinte("Error getting file attribute when initializing file %s!\n", Filename);
        else
        {
            if (FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                Result = assetsys_allocate_file(AssetSys, FileType_Directory);
                
                assetsys_file *File = assetsys_get_file(AssetSys, Result);
                File->Name = mstr_init((char*)Filename, FilenameLen);
                File->HashedName = hash_bytes((void*)Filename, FilenameLen);
                
                assetsys_file_init_recurse_directory(AssetSys, Result, Filename);
            }
            else
            {
                Result = assetsys_allocate_file(AssetSys, FileType_File);
                
                assetsys_file *File = assetsys_get_file(AssetSys, Result);
                File->Name = mstr_init((char*)Filename, FilenameLen);
                File->HashedName = hash_bytes((void*)Filename, FilenameLen);
            }
        }
    }
    
    return Result;
}

file_internal void assetsys_file_free(assetsys_file *File)
{
    File->Type = FileType_Unknown;
    if (File->FileInfo) 
    {
        // TODO(Dustin): 
        
        File->FileInfo = file_id_invalid;
    }
    
    
    if (File->ChildFiles) memory_release(Core->Memory, File->ChildFiles);
    File->ChildFileCount = 0;
    mstr_free(&File->Name);
    File->HashedName.Upper = 0;
    File->HashedName.Lower = 0;
}

file_internal void assetsys_file_pool_init(assetsys_file_pool *FilePool)
{
    FilePool->Handles = (assetsys_file_t)memory_alloc(Core->Memory, 
                                                      sizeof(assetsys_file) * MAX_ASSETSYS_POOL_FILE_COUNT);
    
    FilePool->AllocatedFiles = 0;
    
    for (u32 i = 0; i < MAX_ASSETSYS_POOL_FILE_COUNT; ++i)
    {
        FilePool->Handles[i].Id.Minor = i;
        FilePool->Handles[i].Type = FileType_Unknown;
    }
}

file_internal void assetsys_file_pool_free(assetsys_file_pool *FilePool)
{
    for (u32 i = 0; i < MAX_ASSETSYS_POOL_FILE_COUNT; ++i)
        assetsys_file_free(FilePool->Handles + i);
    
    memory_release(Core->Memory, FilePool->Handles);
    FilePool->AllocatedFiles = 0;
}

// If the pool is full (Pool == NULL), then File will remain NULL.
file_internal void assetsys_file_pool_alloc(assetsys_file_pool *FilePool, assetsys_file **File)
{
    // Don't bother to scan files if the array is full
    if (FilePool->AllocatedFiles + 1 <= MAX_ASSETSYS_POOL_FILE_COUNT)
    {
        // find an open file
        for (u32 i = 0; i < MAX_ASSETSYS_POOL_FILE_COUNT; ++i)
        {
            if (FilePool->Handles[i].Type == FileType_Unknown)
            {
                *File = FilePool->Handles + i;
                
                // Index = 0 is the error case, so always add 1
                (*File)->Id.Minor = i + 1;
                
                ++FilePool->AllocatedFiles;
                break;
            }
        }
    }
}

file_internal void assetsys_file_pool_release(assetsys_file_pool *FilePool, assetsys_file_id Fid)
{
    assetsys_file* Ptr = (assetsys_file*)(FilePool->Handles + Fid.Minor);
    Ptr->Type = FileType_Unknown;
    FilePool->AllocatedFiles--;
}

file_internal void assetsys_build_comparator_list(comparator_list *List, const char *Filepath)
{
    u32 Count = 1;
    char *pch;
    
    // how many directories are there?
    pch = strchr(Filepath, '/');
    
    while (pch != NULL)
    {
        pch = strchr(pch + 1, '/');
        Count++;
    };
    
    // Build the comparator list
    List->Count = Count;
    List->Idx = 0;
    List->Comparators = memory_alloc(Core->Memory, List->Count * sizeof(u128));
    
    pch = NULL;
    pch = strchr(Filepath, '/');
    char *Offset = (char*)Filepath;
    
    while (pch != NULL)
    {
        List->Comparators[List->Idx++] = hash_bytes(Offset, pch - Offset);
        Offset += pch - Offset + 1;
        pch = strchr(pch + 1, '/');
    }
    
    List->Comparators[List->Idx] = hash_bytes(Offset, strlen(Filepath) - (Offset - Filepath));
    List->Idx = 0;
}

file_id assetsys_open(assetsys *AssetSys, const char *Filepath, bool IsRelative, const char *MountName, file_mode Mode)
{
    file_id Result = file_id_invalid;
    
    assetsys_mount_point MountPoint;
    mstr AbsolutePath;
    
    if (IsRelative)
    {
        if (MountName)
        {
            MountPoint = assetsys_find_mount_point(AssetSys, MountName);
        }
        else
        {
            MountPoint = assetsys_find_mount_point(AssetSys, "root");
        }
        
        // TODO(Dustin): Error mount point?
    }
    else
    {
        // TODO(Dustin): 
        // 1. Search for the file with the virtualized file tree
        
        // 2. If it was not found, then create a assetsys_file
        // ---- Create a soft link to the file inside the root directory
        
        // Create a soft link in order to find the file in the future?
        // build\* ...
        // --- C:\Documents\cool_game\file.txt
        
    }
    
    comparator_list CompList = {0};
    assetsys_build_comparator_list(&CompList, Filepath);
    
    assetsys_file_id Fid = assetsys_find_fid(AssetSys, MountPoint.File, &CompList);
    
    assetsys_file *File = assetsys_get_file(AssetSys, Fid);
    if (file_id_is_valid(File->FileInfo))
    {
        // File is already open
        
    }
    
    if (!assetsys_valid_file_id(Fid))
    {
        // NOTE(Dustin): This means the file does not current exist in the filesystem. If the 
        // FileMode is set to Read, then return an invalid file id. Otherwise open the file with 
        // "create" flags
        if (Mode == FileMode_Read)
        {
            mprinte("Could not find the file at the specified mount point! File: \"%s\"\n", Filepath);
            return file_id_invalid;
        }
    }
    
    // Open the file
    
    char Path[2048];
    snprintf(Path, 2048, "%s/%s", mstr_to_cstr(&MountPoint.AbsolutePath), Filepath);
    
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    bool FileDoesNotExist = false;
    
    if (Mode == FileMode_Read)
    {
        FileHandle = CreateFileA(Path, 
                                 GENERIC_READ, 
                                 FILE_SHARE_READ, 
                                 0,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, 0);
    }
    else if (Mode == FileMode_Write || Mode == FileMode_ReadWrite)
    {
        FileHandle = CreateFileA(Path, 
                                 GENERIC_WRITE, 
                                 0, 
                                 0,
                                 TRUNCATE_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
        
        
        // file doesn't currently exist
        if (FileHandle == INVALID_HANDLE_VALUE)
        {
            FileDoesNotExist = true;
        }
    }
    else if (Mode == FileMode_Append)
    {
        FileHandle = CreateFileA(Path, 
                                 FILE_APPEND_DATA, 
                                 FILE_SHARE_READ, 
                                 NULL, // No security
                                 OPEN_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
        
        // file doesn't currently exist
        if (FileHandle == INVALID_HANDLE_VALUE)
        {
            FileDoesNotExist = true;
        }
    }
    
    // For ReadWrite, Write, and Append modes, if the file doesn't exist, then
    // need to create it. if the creation is sucessfull, insert into the virtual
    // file system
    if (FileDoesNotExist)
    {
        FileHandle = CreateFileA(Path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW,
                                 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
        
        if (FileHandle != INVALID_HANDLE_VALUE)
        {
            comparator_list CompList;
            assetsys_build_comparator_list(&CompList, Filepath);
            
            char *Filename = strrchr(Path, '/') + 1;
            u32 FileLen = strlen(Path) - strlen(Filename);
            
            char *Directory = Path;
            u32 DirLen = Filename - Directory - 1;
            
            Fid = assetsys_insert_file_in_tree(AssetSys, 
                                               &CompList, 
                                               MountPoint.File, 
                                               Filename, FileLen,
                                               Directory, DirLen);
        }
    }
    
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        mprint("Unable to open file \"%s\"\n", Path);
        return file_id_invalid;
    }
    
    file_info *FileInfo = NULL;
    void* FileMemory = NULL;
    u32 FileIndex = 0;
    
    for (u32 i = 0; i < 2; ++i)
    {
        // Negate the particular bitset. Ctz will find the first
        // 1 starting from the the least significant digit. However,
        // for the purposes of this bitlist, a 1 means an opened file,
        // but we want to find unopened files. Negating the bitset will
        // set all opened files to 0 and unopened files to 1.
        // The found idx is the index of the file we want to open
        u64 Mask = ~AssetSys->OpenFilesMask[i];
        u32 Bit;
        
        if (Mask)
            Bit = PlatformCtzl(Mask);
        else
            continue;
        
        FileIndex = i * 64 + Bit;
        
        FileInfo = AssetSys->OpenFiles + FileIndex;
        
        BIT_TOGGLE_1(AssetSys->OpenFilesMask[i], FileIndex);
        
        break;
    }
    
    if (!FileInfo)
    {
        mprinte("Unable to open file \"%s\"! Too many open files!\n", Filepath);
        CloseHandle(FileHandle);
        return Result;
    }
    
    FileInfo->Mode         = Mode;
    FileInfo->Handle       = FileHandle;
    FileInfo->Size         = GetFileSize(FileHandle, NULL);
    FileInfo->Memory       = AssetSys->FileMemory + FileIndex; // NOTE(Dustin): Not guaranteed to be initialized
    FileInfo->MemoryOffset = 0;
    FileInfo->FileOffset   = 0;
    FileInfo->Fid          = Fid;
    
    Result = FileIndex;
    
    // Now get the file and set the back pointer to the FileInfo
    File->FileInfo = Result;
    
    return Result;
}

file_id assetsys_load(assetsys *AssetSys, const char *Filepath, bool IsRelative, const char *MountName)
{
    file_id Result = assetsys_open(Core->AssetSys, Filepath, IsRelative, MountName, FileMode_Read);
    
    // TODO(Dustin): Load the file (or part of it) into memory
    
    
    return Result;
}

void assetsys_close(assetsys *AssetSys, file_id Fid)
{
    file_info *FileInfo = AssetSys->OpenFiles + Fid;
    assetsys_file *File = assetsys_get_file(AssetSys, FileInfo->Fid);
    
    if (FileInfo->Memory) FileInfo->Memory = NULL;
    if (FileInfo->Handle) CloseHandle(FileInfo->Handle);
    FileInfo->Size         = 0;
    FileInfo->MemoryOffset = 0;
    FileInfo->FileOffset   = 0;
    FileInfo->Fid          = assetsys_file_id_invalid;
    
    File->FileInfo = file_id_invalid;
    
    
    u32 MaskIndex = Fid / 64;
    u32 BitIndex = Fid % 64;
    BIT_TOGGLE_0(AssetSys->OpenFilesMask[MaskIndex], BitIndex);
}

//~ User API

void file_print_directory_tree(const char *MountName)
{
    assetsys *AssetSys = Core->AssetSys;
    u128 Comparator = hash_bytes((void*)MountName, strlen(MountName));
    
    for (u32 i = 0; i < AssetSys->MountedFilesCount; ++i)
    {
        if (compare_hash(Comparator, AssetSys->MountedFiles[i].Name))
        {
            assetsys_internal_traverse_tree(AssetSys, AssetSys->MountedFiles[i].File, 0);
            break;
        }
    }
}

file_id file_open(const char *Filepath, bool IsRelative, const char *MountName, file_mode Mode)
{
    return assetsys_open(Core->AssetSys, Filepath, IsRelative, MountName, Mode);
}

file_id file_load(const char *Filepath, bool IsRelative, const char *MountName)
{
    return assetsys_load(Core->AssetSys, Filepath, IsRelative, MountName);
}

void file_close(file_id Fid)
{
    assetsys_close(Core->AssetSys, Fid);
}
