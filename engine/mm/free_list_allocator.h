#ifndef MAPLE_MM_FREE_LIST_ALLOCATOR
#define MAPLE_MM_FREE_LIST_ALLOCATOR

typedef struct header* header_t;

typedef struct
{
    u64   Size;

    void *Start;
    void *Brkp;

    header_t FreeList;

    // Memory Usage tracking
    u64 NumAllocations;
    u64 UsedMemory;
} free_allocator;

void FreeListAllocatorInit(free_allocator *Allocator, u64 Size, void* Ptr);
void FreeListAllocatorFree(free_allocator *Allocator);

void* FreeListAllocatorAlloc(free_allocator *Allocator, u64 Size);
void* FreeListAllocatorRealloc(free_allocator *Allocator, void *Ptr, u64 Size);
void FreeListAllocatorAllocFree(free_allocator *Allocator, void *Ptr);

#endif //MAPLE_MM_FREE_LIST_ALLOCATOR
