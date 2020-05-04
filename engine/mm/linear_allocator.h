#ifndef JACKAL_MM_LINEAR_ALLOCATOR
#define JACKAL_MM_LINEAR_ALLOCATOR

/*

A LinearAllocator is an allocator that tracks the
start of the allocator and the next free allocation.
When an allocation is made, then the next free allocation
will be: NextFree += size_of_this_allocation.

Allocations cannot be be freed. In order to free on
 per-allocation basis, the entire allocator has to be
reset. Resetting the allocator simply moves the next free
allocation pointer back to the start of the list. The
memory is not zeroed, so if allocations contain objects
 that have their own allocations, it is up to the user
to free the resources resetting the Allocator.

*/

namespace mm {
    
    class LinearAllocator : public Allocator {
        public:
        
        LinearAllocator(size_t size, void *start);
        ~LinearAllocator();
        
        // Copy and Move not allowed for an allocator
        LinearAllocator(LinearAllocator & other) = delete;
        LinearAllocator(LinearAllocator && other) = delete;
        
        LinearAllocator& operator=(LinearAllocator & other)  = delete;
        LinearAllocator& operator=(LinearAllocator && other) = delete;
        
        virtual void * Allocate(size_t size, size_t alignment)             override;
        virtual void* Reallocate(void *src, size_t size, size_t alignment) override;
        virtual void Free(void * ptr) override;
        
        // resets the linear allocator
        void Reset();
        
        private:
        
        void * NextFree;
    };
    
}; // mm

#endif // JACKAL_MM_LINEAR_ALLOCATOR