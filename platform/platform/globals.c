
globals *Core;

void globals_init(globals_create_info *CreateInfo)
{
    void *PlatformMemory = PlatformRequestMemory(CreateInfo->Memory.Size);
    
    memory Memory = {0};
    memory_init(&Memory, CreateInfo->Memory.Size, PlatformMemory);
    
    memory *pMemory = (memory*)memory_alloc(&Memory, sizeof(memory));
    *pMemory = Memory;
    
    Core = (globals*)memory_alloc(pMemory, sizeof(globals));
    Core->Memory = pMemory;
    
    Core->AssetSys = (assetsys*)memory_alloc(Core->Memory, sizeof(assetsys));
    assetsys_init(Core->AssetSys, (char*)CreateInfo->AssetSystem.ExecutablePath);
    
    for (u32 i = 0; i < CreateInfo->AssetSystem.MountPointsCount; ++i)
    {
        assetsys_mount_point_create_info *MountInfo = CreateInfo->AssetSystem.MountPoints + i;
        assetsys_mount(Core->AssetSys, MountInfo->Path, MountInfo->MountName, MountInfo->IsRelative);
    }
}

void globals_free()
{
    assetsys_free(Core->AssetSys);
    memory_release(Core->Memory, Core->AssetSys);
    
    memory Memory = *Core->Memory;
    void *MemoryPtr = Memory.Start;;
    
    memory_release(&Memory, Core->Memory);
    Core->Memory = NULL;
    
    memory_release(&Memory, Core);
    Core = NULL;
    
    PlatformReleaseMemory(MemoryPtr, 0);
}
