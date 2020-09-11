
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
}

void globals_free()
{
    memory Memory = *Core->Memory;
    void *MemoryPtr = Memory.Start;;
    
    memory_release(&Memory, Core->Memory);
    Core->Memory = NULL;
    
    memory_release(&Memory, Core);
    Core = NULL;
    
    PlatformReleaseMemory(MemoryPtr, 0);
}
