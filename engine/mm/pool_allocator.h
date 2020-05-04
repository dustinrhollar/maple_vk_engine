#ifndef JENGINE_MM_POOL_ALLOCATOR
#define JENGINE_MM_POOL_ALLOCATOR

/*

A PoolAllocator allocates an element of a particular
 size and alignment. Only one size/alignment is allowed
for the allocator.

The minimum allowed size of an element in the Pool
Allocator is the size of a void* (on 64bit system it will
be 8 bytes). This is so that an implicit header can be
used for allocations. The Pool Allocator maintains an
implicit stack of freed allocations. The stack only
needs to know about the next element in the list, so
if allocations are at least the size of a void* the
memory of the allocation can be converted into the
next pointer.

Freed allocations are added to the top of the stack
and new allocations are removed from the top of
the stack.

*/

namespace mm {
    
    class PoolAllocator : public Allocator
    {
        public:
        
        PoolAllocator(size_t size, void *start, size_t allocation_size, u8 alignment);
        ~PoolAllocator();
        
        // Copy and Move not allowed for an allocator
        PoolAllocator(PoolAllocator & other) = delete;
        PoolAllocator(PoolAllocator && other) = delete;
        
        PoolAllocator& operator=(PoolAllocator & other) = delete;
        PoolAllocator& operator=(PoolAllocator && other) = delete;
        
        virtual void *Allocate(size_t size, size_t alignment) override;
        virtual void* Reallocate(void *src, size_t size, size_t alignment) override;
        virtual void Free(void * ptr) override;
        
        private:
        
        size_t AllocationSize;
        u8 Alignment;
        
        void **FreeList;
    };
    
} // mm

#endif // JENGINE_MM_POOL_ALLOCATOR