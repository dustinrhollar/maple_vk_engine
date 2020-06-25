#ifndef MAPLE_MM_H
#define MAPLE_MM_H

template<typename T>
T* palloc(free_allocator *Allocator, u32 NumElements)
{
    return (T*)FreeListAllocatorAlloc(Allocator, sizeof(T) * NumElements);
}

template<typename T>
void pfree(free_allocator *Allocator, T *Ptr)
{
    FreeListAllocatorAllocFree(Allocator, (void*)Ptr);
}

template<typename T>
T* halloc(tag_block_t Allocator, u64 Size)
{
    return (T*)TaggedHeapBlockAlloc(Allocator, sizeof(T) * Size);
}


#endif //MAPLE_MM_H