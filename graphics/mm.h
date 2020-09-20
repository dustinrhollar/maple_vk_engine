#ifndef MAPLE_MM_H
#define MAPLE_MM_H

template<typename T>
T* palloc(memory *Allocator, u32 NumElements = 1)
{
    return (T*)memory_alloc(Allocator, sizeof(T) * NumElements);
}

template<typename T>
T* palloc(u32 NumElements = 1)
{
    return (T*)memory_alloc(Core->Memory, sizeof(T) * NumElements);
}

template<typename T>
void pfree(memory *Allocator, T *Ptr)
{
    memory_release(Allocator, (void*)Ptr);
}


template<typename T>
void pfree(T *Ptr)
{
    memory_release(Core->Memory, (void*)Ptr);
}

#if 0
template<typename T>
T* halloc(tag_block_t Allocator, u64 Size)
{
    return (T*)TaggedHeapBlockAlloc(Allocator, sizeof(T) * Size);
}
#endif

#endif //MAPLE_MM_H