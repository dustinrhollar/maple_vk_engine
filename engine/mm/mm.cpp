
// TODO(Dustin): Use VirtualAlloc instead of malloc when the platform layer in reinstated

namespace mm {
    
    file_global void *GlobalMemory = nullptr;
    
    file_global FreeListAllocator *PermanantStorage = nullptr;
    file_global LinearAllocator   *TransientStorage = nullptr;
    
    // Initializes the memory manager and the Permanant/Transient storage allocators.
    // @param memory_to_reserve: total memory that is allocated from the operating system
    // @param memory_for_transient_storage: memory that is allocated from total storage for
    // transient storage. The remaining memory is used for Permanant Storage
    void InitializeMemoryManager(size_t memory_to_reserve, size_t memory_for_transient_storage)
    {
        assert(memory_to_reserve > memory_for_transient_storage && "When intializing Memory Manager, Total Memory should be larger than Transient Memory");
        
        size_t size_of_free_list = sizeof(FreeListAllocator);
        size_t size_of_linear_allocator = sizeof(LinearAllocator);
        
        // offset for linear allocator
        size_t linear_offset = size_of_free_list;
        // offset into memory block for transient storage
        size_t transient_offset = linear_offset + size_of_linear_allocator;
        // offset into memory for the permanant storage
        size_t permanant_offset = transient_offset + memory_for_transient_storage;
        
        // Since Permanant/Transient storage are pointers, need to add space
        // for the actual objects to live in. They will be placed in the front
        // of the allocation:
        // ---------------------------------------------
        // | Permanant Object | Transient Object | ... |
        // ---------------------------------------------
        size_t total_memory = size_of_free_list + size_of_linear_allocator + memory_to_reserve;
        GlobalMemory = malloc(total_memory);
        
        void *permanant_start = (char*)GlobalMemory + permanant_offset;
        void *transient_start = (char*)GlobalMemory + transient_offset;
        
        PermanantStorage = new (GlobalMemory) FreeListAllocator(memory_to_reserve - memory_for_transient_storage,
                                                                permanant_start);
        TransientStorage = new ((char*)GlobalMemory+linear_offset) LinearAllocator(memory_for_transient_storage,
                                                                                   transient_start);
    }
    
    
    void ShutdownMemoryManager()
    {
        /*

        While it is not necessary to explicitly call the destructors
        for Transient/Permanant Storage, it is done to check for memory
        leaks in the program. If all allocations from Transient/Permanant
        Storage are not freed by the program, then an assertion is thrown
        by the destructor.

        */
        
        TransientStorage->Reset();
        TransientStorage->~LinearAllocator();
        
        PermanantStorage->~FreeListAllocator();
        
        free(GlobalMemory);
    }
    
    void ResetTransientMemory()
    {
        TransientStorage->Reset();
    }
    
    // Allocates raw memory from main allocators: Permanent/Transient
    // By default, requests go to Permanant storage
    void* jalloc(size_t size, MemoryRequest memory_request)
    {
        switch (memory_request)
        {
            case MEMORY_REQUEST_PERMANANT_STORAGE:
            {
                return PermanantStorage->Allocate(size, __alignof(DEFAULT_ALIGNMENT));
            } break;
            case MEMORY_REQUEST_TRANSIENT_STORAGE:
            {
                return TransientStorage->Allocate(size, __alignof(DEFAULT_ALIGNMENT));
            } break;
            default: return nullptr;
        }
    }
    
    // Frees raw memory from the main allocators: Permanent/Transient
    // By default, requests go to Permanant storage
    void jfree(void *ptr, MemoryRequest memory_request)
    {
        switch (memory_request)
        {
            case MEMORY_REQUEST_PERMANANT_STORAGE:
            {
                PermanantStorage->Free(ptr);
            } break;
            case MEMORY_REQUEST_TRANSIENT_STORAGE:
            {
                TransientStorage->Free(ptr);
            } break;
            default: break;
        }
    }
    
    
    void *jrealloc(void *ptr, size_t size, MemoryRequest memory_request)
    {
        switch (memory_request)
        {
            case MEMORY_REQUEST_PERMANANT_STORAGE:
            {
                return PermanantStorage->Reallocate(ptr, size, __alignof(DEFAULT_ALIGNMENT));
            } break;
            case MEMORY_REQUEST_TRANSIENT_STORAGE:
            {
                return TransientStorage->Reallocate(ptr, size, __alignof(DEFAULT_ALIGNMENT));
            } break;
            default: return nullptr;
        }
    }
    
    Allocator *GetPermanantStorage() {
        return PermanantStorage;
    }
    
} // mm

void *talloc(size_t size) {
    return mm::jalloc(size, mm::MEMORY_REQUEST_TRANSIENT_STORAGE);
}

void *palloc(size_t size) {
    //return malloc(size);
    return mm::jalloc(size, mm::MEMORY_REQUEST_PERMANANT_STORAGE);
}

void *prealloc(void *src, size_t size)
{
    return mm::jrealloc(src, size, mm::MEMORY_REQUEST_PERMANANT_STORAGE);
}


void *trealloc(void *src, size_t size)
{
    return mm::jrealloc(src, size, mm::MEMORY_REQUEST_TRANSIENT_STORAGE);
}
