
namespace mm {
    
    LinearAllocator::LinearAllocator(size_t size, void *start)
        : Allocator(size, start)
        , NextFree(StartAddr)
    {
    }
    
    LinearAllocator::~LinearAllocator()
    {
        NextFree = nullptr;
    }
    
    void* LinearAllocator::Allocate(size_t size, size_t alignment)
    {
        u8 align_length = AlignLength(NextFree, alignment);
        size_t adj_size = align_length + size;
        
        // not enough memory for allocation
        if (adj_size + UsedMemory > Size)
        {
            printf("Not enough memory for allocation in Linear Allocator!\n");
            return nullptr;
        }
        
        void *addr = NextFree;
        NextFree = (void*)((char*)NextFree + adj_size);
        
        UsedMemory += adj_size;
        ++NumAllocations;
        
        return (void*)((char*)addr + align_length);
    }
    
    void LinearAllocator::Free(void * ptr)
    {
        assert(false && "Free should not be called on a Linear Allocator.");
    }
    
    void LinearAllocator::Reset()
    {
        UsedMemory = 0;
        NumAllocations = 0;
        
        NextFree = StartAddr;
    }
    
    void* LinearAllocator::Reallocate(void *src, size_t size, size_t alignment) {
        void *result = Allocate(size, alignment);
        memcpy(result, src, size);
        
        return result;
    }
    
}; // mm
