
#define MOUNT_ROOT_IDX 0
// Minor Idx = 0 is the invalid case, so always subtract by one when indexing
#define assetsys_get_file(sys, id) sys->FilePool[id.Major].Handles + (id.Minor - 1)
#define assetsys_valid_file_id(id) (id.Minor != 0) 

// When looking for a file id, it will be common that the 
// file is nested within several directories. However, the absolute paths
// of the directories are not stored. Therefore, preprocess the filepath
// into a list of comparators and search for the directories.
typedef struct comparator_list
{
    u32   Count;
    u32   Idx;
    u128 *Comparators;
}comparator_list;

// Error functions
file_internal assetsys_error assetsys_translate_win32_error(u32 ErrorCode);
file_internal void assetsys_get_err_str(char *Buffer, u32 BufferLen, assetsys_error Error);

// tree traversals
file_internal void assetsys_internal_traverse_tree(assetsys *AssetSys, file_id Fid, u32 Depth);
file_internal file_id assetsys_find_fid(assetsys *AssetSys, file_id CurrentId, comparator_list *CompList);
file_internal void assetsys_file_init_recurse_directory(assetsys *AssetSys, file_id ParentFid, const char *Filepath);

// File
file_internal file_id assetsys_allocate_file(assetsys *AssetSys, assetsys_file_type FileType);
file_internal file_id assetsys_file_init(assetsys *AssetSys, const char *Filename, u32 FilenameLen, 
                                         bool IsRelative, const char *DirPath, u32 DirPathLen);
file_internal void assetsys_file_free(assetsys_file *File);

// File Pool
file_internal void assetsys_file_pool_init(assetsys_file_pool *FilePool);
file_internal void assetsys_file_pool_free(assetsys_file_pool *FilePool);
file_internal void assetsys_file_pool_alloc(assetsys_file_pool *FilePool, assetsys_file **File);
file_internal void assetsys_file_pool_release(assetsys_file_pool *FilePool, file_id Fid);

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

file_internal void assetsys_internal_traverse_tree(assetsys *AssetSys, file_id Fid, u32 Depth)
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
    
    // Mount the root directory, will initialize the file tree for the root directory
    assetsys_mount(AssetSys, mstr_to_cstr(&AssetSys->RootStr), "root", false);
    
    // let's traverse the directory tree to verify results
    //assetsys_internal_traverse_tree(AssetSys, AssetSys->MountedFiles[0].File, 0);
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

file_internal file_id assetsys_find_fid(assetsys *AssetSys, file_id CurrentId, comparator_list *CompList)
{
    file_id Result = file_id_invalid;
    
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

// Mount a file (file/directory/zip) from a name
void assetsys_mount(assetsys *AssetSys, const char *Filename, const char *MountName, bool IsRelative)
{
    if (!IsRelative)
    {
        // 1. If the Root Directory has not been mounted, mount it
        if (AssetSys->MountedFilesCount == 0)
        {
            file_id Root = assetsys_file_init(AssetSys, Filename, strlen(Filename), false, NULL, 0);
            
            assetsys_mount_point *Mount = AssetSys->MountedFiles + MOUNT_ROOT_IDX;
            
            // NOTE(Dustin): For now, all mounts will be directories
            // TODO(Dustin): Routine for getting file type
            Mount->Type = MountType_Directory;
            Mount->Name = hash_bytes((void*)MountName, strlen(MountName));
            Mount->File = Root;
            
            AssetSys->MountedFilesCount++;
        }
        else
        {
            // 2. Determine if the directory matches the root directory. If it
            // is part of the root directory, then files have been created
            // already. Find the file_id to mount it.
            
            // 3. If lies outside of the root directory, load the file directory
            //    for this mount point. Will have to recursively create the files
            //    like the root did.
        }
    }
    else
    {
        // File is relative to the root, files are already allocated.
        // Find the file_id to mount it.
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
        file_id RootFid = AssetSys->MountedFiles[MOUNT_ROOT_IDX].File;
        file_id MountFid = assetsys_find_fid(AssetSys, RootFid, &CompList);
        
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
            
            AssetSys->MountedFiles[AssetSys->MountedFilesCount++] = Mount;
        }
        else
        {
            mprinte("Unable to mount \"%s\"!\n", Filename);
        }
    }
}

// Mount a file (file/directory/zip) from a file pointer
void assetsys_mountf(assetsys *AssetSys, file_t File)
{
    
}

file_internal file_id assetsys_allocate_file(assetsys *AssetSys, assetsys_file_type FileType)
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
            File->Handle         = INVALID_HANDLE_VALUE;
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
            File->Handle         = INVALID_HANDLE_VALUE;
            File->ChildFiles     = NULL;
            File->ChildFileCount = 0;
            
            AssetSys->FilePoolCount++;
        }
    }
    
    return File->Id;
}

file_internal void assetsys_file_init_recurse_directory(assetsys *AssetSys, file_id ParentFid, const char *Filepath)
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
    ParentFile->ChildFiles = (file_id*)memory_alloc(Core->Memory, sizeof(file_id) * ParentFile->ChildFileCount);
    
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
                
                file_id ChildFid = assetsys_file_init(AssetSys, 
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

file_internal file_id assetsys_file_init(assetsys *AssetSys, const char *Filename, u32 FilenameLen, 
                                         bool IsRelative, const char *DirPath, u32 DirPathLen)
{
    file_id Result = {0};
    
    BY_HANDLE_FILE_INFORMATION FileInfo;
    BOOL Err;
    
    // If the filepath is a relative path, need to build the fullpath based on the
    // mountname - if one was provided
    if (IsRelative)
    {
        char Path[2048];
        
        if (DirPath)
        {
            snprintf(Path, 2048, "%s/%s", DirPath, Filename);
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
    if (File->Handle != INVALID_HANDLE_VALUE) CloseHandle(File->Handle);
    if (File->ChildFiles) memory_release(Core->Memory, File->ChildFiles);
    File->ChildFileCount = 0;
    mstr_free(&File->Name);
    File->HashedName.Upper = 0;
    File->HashedName.Lower = 0;
}

file_internal void assetsys_file_pool_init(assetsys_file_pool *FilePool)
{
    FilePool->Handles = (asset_file_t)memory_alloc(Core->Memory, 
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

file_internal void assetsys_file_pool_release(assetsys_file_pool *FilePool, file_id Fid)
{
    assetsys_file* Ptr = (assetsys_file*)(FilePool->Handles + Fid.Minor);
    Ptr->Type = FileType_Unknown;
    FilePool->AllocatedFiles--;
}

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
