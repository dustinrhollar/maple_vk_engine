#include "proxy_allocator.h"

namespace mm {
    
    ProxyAllocator::ProxyAllocator(Allocator &allocator)
        : Allocator(allocator.GetSize(), allocator.GetStart())
        , InternalAllocator(allocator)
    {
    }
    
    ProxyAllocator::~ProxyAllocator()
    {
    }
    
    void * ProxyAllocator::Allocate(size_t size, size_t alignment)
    {
        NumAllocations++;
        size_t mem = InternalAllocator.GetUsedMemory();
        
        void *ptr = InternalAllocator.Allocate(size, alignment);
        UsedMemory += InternalAllocator.GetUsedMemory() - mem;
        
        return ptr;
    }
    
    void* ProxyAllocator::Reallocate(void *src, size_t size, size_t alignment)
    {
        void *result = InternalAllocator.Reallocate(src, size, alignment);
        
        size_t old_used = UsedMemory;
        size_t old_allocations = NumAllocations;
        
        size_t new_used = InternalAllocator.GetUsedMemory();
        size_t new_allocations= InternalAllocator.GetNumAllocations();
        
        UsedMemory += new_used - old_used;
        NumAllocations = new_allocations - old_allocations;
        
        return result;
    }
    
    void ProxyAllocator::Free(void * ptr)
    {
        NumAllocations--;
        size_t mem = InternalAllocator.GetUsedMemory();
        
        InternalAllocator.Free(ptr);
        UsedMemory -= mem - InternalAllocator.GetUsedMemory();
    }
    
} // mm