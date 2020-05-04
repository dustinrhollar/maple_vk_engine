#ifndef JENGINE_MM_FREE_LIST_ALLOCATOR_H
#define JENGINE_MM_FREE_LIST_ALLOCATOR_H

/*

FreeListAllocator maintains an implicit doubly
linked list that tracks freed allocations. Freed
allocations are called "blocks" in the linked list.

When allocating memory, the free list is searched
for a block  of suitable size using first fit search.
If a found block has a size more than 24bytes of extra
space, then the found block is split and where the leftover
memory is inserted back into the free list. If a block
 of suitable size is not found, then nullptr is returned.

Freed allocations are inserted into the freed list based
on their start address. If the previous and next block
are adjacent to the new block, then the blocks are merged
in order to reduve internal fragmentation.

A header for an allocation contains the following information:
1. Size of allocation: alignment/header + requested size
2. (Allocation Only) aligned length of the allocation, which is a mixture
 of alginment and the header.
3. (FreeList Only) Pointer to the next free block of memory
4. (FreeList Only) Pointer to the previous free block of memory

Alginment is only stored for used allocations and the next/previous
pointers are only stored for freed allocations.

Total size of the header is 24 bytes.

*/

namespace mm {
    
    class FreeListAllocator : public Allocator
    {
        public:
        
        FreeListAllocator(size_t size, void *start);
        ~FreeListAllocator();
        
        // Quick reset of the free list allocator
        void ReleaseMemory();
        
        // Copy and Move not allowed for an allocator
        FreeListAllocator(FreeListAllocator & other) = delete;
        FreeListAllocator(FreeListAllocator && other) = delete;
        
        FreeListAllocator& operator=(FreeListAllocator & other) = delete;
        FreeListAllocator& operator=(FreeListAllocator && other) = delete;
        
        virtual void* Allocate(size_t size, size_t alignment)              override;
        virtual void* Reallocate(void *src, size_t size, size_t alignment) override;
        virtual void Free(void * ptr)                                      override;
        
        private:
        
        // For allocated blocks, { Size, Alignment } is used
        // For free blocks, { Size, Next, Prev } is used
        struct Header {
            size_t Flags;
            Header *Next;
            Header *Prev;
        };
        using header_t = Header;
        
        inline Header* Mem2Header(void *mem);
        inline void* Header2Mem(Header *header);
        
        void coalesce(header_t *left_header, header_t *right_header);
        header_t* split(header_t *header, size_t size);
        
        inline void free_list_add(header_t *header);
        inline void free_list_remove(header_t *header);
        
        header_t *FindFreeHeader(size_t size);
        
        char *_brkp;
        char *_endp;
        
        Header *FreeList;
        size_t FreeListSize;
    };
    
} // mm

#endif // JENGINE_MM_FREE_LIST_ALLOCATOR_H
