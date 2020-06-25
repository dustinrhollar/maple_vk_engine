
#define BLOCK_SIZE 8
#define MemAlign(n) ((n) + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1)

void PoolAllocatorInit(pool_allocator *Allocator, void *MemoryPtr, u64 Size, u32 Stride)
{
    Allocator->UsedMemory     = 0;
    Allocator->NumAllocations = 0;
    Allocator->Size           = MemAlign(Size);
    Allocator->Stride         = Stride;
    Allocator->Start          = MemoryPtr;
    Allocator->Pool           = (void**)Allocator->Start;
    
    u64 NumObjects = Size / Stride;
    void **Iter = Allocator->Pool;
    
    // Assign the next pointers in the free list
    for (u32 i = 0; i < NumObjects; ++i)
    {
        *Iter = (char*)Iter + Allocator->Stride;
        Iter = (void**)(*Iter);
    }
    
    *Iter = NULL;
}

void PoolAllocatorFree(pool_allocator *Allocator)
{
    Allocator->Size     = 0;
    Allocator->Stride   = 0;
    Allocator->Start    = NULL;
    Allocator->Pool     = NULL;
}

void* PoolAllocatorAlloc(pool_allocator *Allocator)
{
    void *Result = NULL;
    
    if (Allocator->Pool)
    {
        Result = Allocator->Pool;
        Allocator->Pool = (void**)(*Allocator->Pool);
        
        Allocator->UsedMemory += Allocator->Stride;
        Allocator->NumAllocations++;
    }
    
    return Result;
}

void PoolAllocatorFree(pool_allocator *Allocator, void *Ptr)
{
    // Insert into the free list
    *((void**)Ptr) = Allocator->Pool;
    Allocator->Pool = (void**)Ptr;
    
    Allocator->UsedMemory -= Allocator->Stride;
    Allocator->NumAllocations--;
}

#undef BLOCK_SIZE
#undef MemAlign
