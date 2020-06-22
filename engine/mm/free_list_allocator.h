#ifndef MAPLE_MM_FREE_LIST_ALLOCATOR
#define MAPLE_MM_FREE_LIST_ALLOCATOR

struct header;
typedef struct header* header_t;

typedef struct free_allocator
{
    u64   Size;

    void *Start;
    void *Brkp;

    header_t FreeList;

} free_allocator;

void FreeListAllocatorInit(free_allocator *Allocator, u64 Size, void*(*Alloc)(u64 Size));
void FreeListAllocatorFree(free_allocator *Allocator, void (*Free)(void *Ptr, u64 Size));

#endif //MAPLE_MM_FREE_LIST_ALLOCATOR
