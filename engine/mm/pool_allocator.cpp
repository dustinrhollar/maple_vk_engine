
namespace mm {
    
    PoolAllocator::PoolAllocator(size_t size, void *start, size_t allocation_size, u8 alignment)
        : Allocator(size, start)
        , AllocationSize(allocation_size)
    {
        assert(allocation_size >= sizeof(void*));
        
        Alignment = AlignLength(start, alignment);
        FreeList = (void**)((char*)StartAddr + Alignment);
        
        size_t num_objects = (size-alignment) / AllocationSize;
        
        // initialize the free list.
        void **iter = FreeList;
        for (int i = 0; i < num_objects-1; ++i)
        {
            *iter = (char*)iter + AllocationSize;
            iter = (void**)(*iter);
        }
        *iter = nullptr;
    }
    
    PoolAllocator::~PoolAllocator()
    {
        FreeList = nullptr;
    }
    
    void* PoolAllocator::Allocate(size_t size, size_t alignment)
    {
        assert(size % AllocationSize == 0 && "Allocation size for the PoolAllocator should be a multiple of AllocationSize.");
        
        if (FreeList == nullptr) return nullptr;
        
        void *ptr = FreeList;
        FreeList = (void**)(*FreeList);
        
        UsedMemory += AllocationSize;
        ++NumAllocations;
        
        return ptr;
    }
    
    void* PoolAllocator::Reallocate(void *src, size_t size, size_t alignment)
    {
        assert(1 && "Cannot call realloc on this allocator!\n");
        return nullptr;
    }
    
    void PoolAllocator::Free(void * ptr)
    {
        *((void**)ptr) = FreeList;
        FreeList = (void**)ptr;
        UsedMemory -= AllocationSize;
        --NumAllocations;
    }
    
} // mm