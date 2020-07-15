
renderer_t         GlobalRenderer;
resource_registry *GlobalResourceRegistry;
free_allocator     GlobalMemory;
tagged_heap        GlobalTaggedHeap;
free_allocator     GlobalStringArena;
asset_registry     GlobalAssetRegistry;

file_global void  *GlobalMemoryPtr;
file_global void  *StringArenaPtr;

void InitializeGlobals(globals_create_info *CreateInfo)
{
    //~ Memory Setup
    
    GlobalMemoryPtr = PlatformRequestMemory(CreateInfo->GlobalMemory.Size);
    
    // Initialize memory manager
    FreeListAllocatorInit(&GlobalMemory, CreateInfo->GlobalMemory.Size, GlobalMemoryPtr);
    
    // Tagged heap. 3 Stages + 1 Resource/Asset = 4 Tags / frame
    // Keep 3 Blocks for each Tag, allowing for 12 Blocks overall
    TaggedHeapInit(&GlobalTaggedHeap, 
                   &GlobalMemory, 
                   CreateInfo->TaggedHeap.Size, 
                   CreateInfo->TaggedHeap.SizePerBlock, 
                   CreateInfo->TaggedHeap.ActiveTags);
    
    StringArenaPtr = FreeListAllocatorAlloc(&GlobalMemory, CreateInfo->StringArena.Size);
    FreeListAllocatorInit(&GlobalStringArena, CreateInfo->StringArena.Size, StringArenaPtr);
    
    
    //~ Setup Graphics
    
    GlobalResourceRegistry = palloc<resource_registry>(&GlobalMemory);
    ResourceRegistryInit(GlobalResourceRegistry, &GlobalMemory, CreateInfo->ResourceRegistry.MaxCount);
    
    RendererInit(&GlobalMemory,
                 GlobalResourceRegistry,
                 CreateInfo->Renderer.Window,
                 CreateInfo->Renderer.Width,
                 CreateInfo->Renderer.Height,
                 CreateInfo->Renderer.RefreshRate);
    
    AssetRegistryInit(&GlobalAssetRegistry, 
                      GlobalRenderer, 
                      &GlobalMemory, 
                      CreateInfo->AssetRegistry.MaxCount);
}

void FreeGlobals()
{
    RendererShutdown(&GlobalMemory);
    AssetRegistryFree(&GlobalAssetRegistry, &GlobalMemory);
    
    ResourceRegistryFree(GlobalResourceRegistry, &GlobalMemory);
    pfree<resource_registry>(&GlobalMemory, GlobalResourceRegistry);
    
    FreeListAllocatorFree(&GlobalStringArena);
    FreeListAllocatorAllocFree(&GlobalMemory, StringArenaPtr);
    
    // Free up the tagged heap for per-frame info
    TaggedHeapFree(&GlobalTaggedHeap, &GlobalMemory);
    
    // free up the global memory
    FreeListAllocatorFree(&GlobalMemory);
    PlatformReleaseMemory(GlobalMemoryPtr, 0);
}
