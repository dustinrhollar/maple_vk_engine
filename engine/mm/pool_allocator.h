#ifndef ENGINE_MM_POOL_ALLOCATOR_H
#define ENGINE_MM_POOL_ALLOCATOR_H

struct pool_allocator
{
    u64 Size;
    u32 Stride;
    
    void *Start;
    void **Pool;
    
    // TODO(Dustin): Wrap in debug deine?
    // Memory Usage tracking
    u64 NumAllocations;
    u64 UsedMemory;
};

void PoolAllocatorInit(pool_allocator *Allocator, void *MemoryPtr, u64 Size, u32 Stride);
void PoolAllocatorFree(pool_allocator *Allocator);

void* PoolAllocatorAlloc(pool_allocator *Allocator);
void  PoolAllocatorFree(pool_allocator *Allocator, void *Ptr);


#endif //ENGINE_MM_POOL_ALLOCATOR_H
