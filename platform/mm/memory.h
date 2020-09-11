#ifndef ENGINE_MM_MEMORY_H
#define ENGINE_MM_MEMORY_H

typedef struct header* header_t;

typedef struct memory
{
    u64   Size;
    
    void *Start;
    void *Brkp;
    
    header_t FreeList;
    
    // Memory Usage tracking
    u64 NumAllocations;
    u64 UsedMemory;
} memory;

void memory_init(memory *Memory, u64 Size, void *Ptr);
void memory_free(memory *Memory);

void* memory_alloc(memory *Memory, u64 Size);
void* memory_realloc(memory *Memory, void *Ptr, u64 Size);
void memory_release(memory *Memory, void *Ptr);

#endif //MEMORY_H
