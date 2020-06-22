
#define BLOCK_SIZE       8
#define HEADER_SIZE      BLOCK_SIZE
#define MIN_LINK_SIZE    sizeof(void*)
#define LIST_HEADER_SIZE 2*MIN_LINK_SIZE

#define MemAlign(n) ((n) + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1)

typedef struct
{
    u64 Size:63;
    u64 Used:1;

    header_t Next;
    header_t Prev;
} header;

void FreeListAllocatorInit(free_allocator *Allocator, u64 Size, void* (*Alloc)(u64 Size))
{
    Allocator->Size = MemAlign(Size);

    Allocator->Start = Alloc(Allocator->Size);
    if (!Allocator->Start)
    {
        Allocator->Size = 0;
    }
    else
    {
        Allocator->Brkp = Allocator->Start;
        Allocator->FreeList = NULL;
    }
}

void FreeListAllocatorFree(free_allocator *Allocator, void (*Free)(void *Ptr, u64 Size))
{
    if (Allocator->Start) Free(Allocator->Start, Allocator->Size);
    Allocator->Start    = NULL;
    Allocator->Brkp     = NULL;
    Allocator->FreeList = NULL;
    Allocator->Size     = 0;
}





#undef MemAlign
#undef LIST_HEADER_SIZE
#undef MIN_LINK_SIZE
#undef HEADER_SIZE
#undef BLOCK_SIZE
