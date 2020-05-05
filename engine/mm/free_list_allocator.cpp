
namespace mm {
    
#define BLOCK_SIZE 8
#define HEADER_SIZE      BLOCK_SIZE
#define MIN_LINK_SIZE    sizeof(void*)
#define MIN_HEADER_SIZE  sizeof(size_t)
#define LIST_HEADER      2*MIN_LINK_SIZE
    
    // Align a size request
#define ALIGN(n) (((n) + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1))
    
    inline size_t FreeListAllocator::HeaderAdjustedSize(u64 n)
    {
        u64 result;
        if (n >= LIST_HEADER)
            result = n;
        else
            result = LIST_HEADER;
        
        return result + MIN_HEADER_SIZE;
    }
    
    void FreeListAllocator::Coalesce(header_t *left_header, header_t *right_header)
    {
        if (!left_header || !right_header)
            return;
        
        // i arbitrarily decided that h1 will be the header
        // that is kept.
        if (left_header < right_header)
        {
            left_header->Next = right_header->Next;
            if (right_header->Next) right_header->Next->Prev = left_header;
        }
        else
        {
            left_header->Prev = right_header->Prev;
            if (right_header->Prev) right_header->Prev->Next = left_header;
        }
        
        // h1_sz + h2_sz will result in the TOTAL memory retrieved.
        // in reality, the memory that can be used is dependent on its
        // size. Looks something like:
        // TOTAL MEMORY = HEADER 1 + H1_SZ + HEADER 2 + H2SZ
        //
        // However, [HEADER 1] will be reserved during a merge, so cannot
        // be considered part of block size.
        //
        // Therefore, we can think of the new block size to be something like:
        //     NEW BLOCK SIZE =  H1_SZ + HEADER 2 + H2SZ
        //     NEW BLOCK SIZE =  H1_SZ + header_adjusted_size(header2)
        left_header->Size += HeaderAdjustedSize(right_header->Size);
    }
    
    FreeListAllocator::header_t* FreeListAllocator::Split(header_t *header, u64 size)
    {
        /*
     Determine if a header can be split

        Let's consider the minimum scenerio where size is 8bytes.
        The Header Alignment uses 24bytes, so an 8byte request is
        aligned to 24bytes.
     
        Since 8bytes is the minimum request size, that means after
        a split, there must be 8 bytes leftover. So a split can occur
        when:
     
        header_size - header_adjusted_size(size) >= 8 bytes
     
        So the new block will have a total allocated size of at least
        24 bytes. 16 bytes of this is usable. 8 bytes is reserved for
        header flags. So new block size will be:
     
        block_size = header_adjusted_size(size) - 8 bytes

Before split:
-----------------------------
| 8b |  x >= 16b            |
-----------------------------

After split:
     -----------------------------
| 8b | size | 8b | leftover |
-----------------------------
     
        */
        
        // size required to form a new block with the requested size
        u64 req_size         = HeaderAdjustedSize(size);
        u64 header_data_size = header->Size;
        //u64 header_data_size = data_size(header);
        
        // Requested memory is over budget
        if (req_size >= header_data_size) return header;
        
        u64 leftover  = header_data_size - req_size;
        // not enough leftover space for an allocation
        if (leftover < HEADER_SIZE) {
            return header;
        }
        
        //data_set_size(header, size); // size being requested
        header->Size = size;
        
        header_t *split_header = (header_t*)((char*)header + req_size);
        // leftover represents the TOTAL space leftover. Need to account for
        // the reserved flags space, so subtract 8bytes
        split_header->Size = leftover - BLOCK_SIZE;
        split_header->Used = 0;
        
        // Add the split header to the free list for future use
        FreeListAdd(split_header);
        
        return header;
    }
    
    inline void FreeListAllocator::FreeListAdd(header_t *header)
    {
        header->Next = nullptr;
        header->Prev = nullptr;
        
        // init the list
        if (FreeList == nullptr)
        {
            FreeList = header;
            FreeList->Prev = nullptr;
            FreeList->Next = nullptr;
            
            return;
        }
        
        header_t *iter = FreeList;
        
        // add to the front
        if (header < FreeList)
        {
            header->Next = FreeList;
            FreeList->Prev = header;
            FreeList = header;
        }
        else
        {
            while (iter->Next != nullptr && header > iter->Next)
            {
                if (iter == header || iter->Next == header) {
                    return;
                }
                
                iter = iter->Next;
            }
            
            // insert the header into the list
            header->Prev = iter;
            header->Next = iter->Next;
            
            if (iter->Next)
            {
                iter->Next->Prev = header;
            }
            
            iter->Next = header;
        }
        
        // If the new node is besides already existing nodes, merge them together
        iter = header;
        
        if (iter->Next)
        {
            u64 adj = HeaderAdjustedSize(iter->Size);
            if (((char*)iter + adj) == (char*)iter->Next)
            {
                Coalesce(iter, iter->Next);
            }
        }
        
        if (iter->Prev)
        {
            u64 adj = HeaderAdjustedSize(iter->Prev->Size);
            if (((char*)iter - adj) == (char*)iter->Prev)
            {
                Coalesce(iter->Prev, iter);
            }
        }
    }
    
    FreeListAllocator::header_t* FreeListAllocator::FindFreeHeader(u64 size)
    {
        header_t *header = FreeList;
        
        int count = 0;
        while (header != nullptr)
        {
            if (!header->Used && size <= header->Size)
            {
                break;
            }
            
            count ++;
            header = header->Next;
        }
        
        if (header) {
            FreeListRemove(header);
        }
        
        return header;
    }
    
    inline void FreeListAllocator::FreeListRemove(header_t *header)
    {
        if (FreeList == nullptr || header == nullptr) return;
        
        if (FreeList == header)
        {
            FreeList = header->Next;
            if (FreeList) FreeList->Prev = nullptr;
        }
        
        // Remove it from the free list
        if (header->Prev != nullptr)
        {
            header->Prev->Next = header->Next;
        }
        
        if (header->Next != nullptr)
        {
            header->Next->Prev = header->Prev;
        }
        
        header->Prev = nullptr;
        header->Next = nullptr;
    }
    
    
    inline FreeListAllocator::header_t* FreeListAllocator::Mem2Header(void *mem) {
        return (header_t*)((char*)mem - MIN_HEADER_SIZE);
    }
    
    inline void* FreeListAllocator::Header2Mem(header_t *header) {
        return (void*)((char*)header + MIN_HEADER_SIZE);
    }
    
    
    FreeListAllocator::FreeListAllocator(u64 size, void *start)
        : Allocator(size, start)
        , FreeList(nullptr)
    {
        assert(size > sizeof(Header));
        
        _brkp = (char*)StartAddr;
        _endp = _brkp + Size;
        
        FreeListSize = 0;
    }
    
    FreeListAllocator::~FreeListAllocator()
    {
        FreeList = nullptr;
        
        _brkp = nullptr;
        _endp = _brkp;
    }
    
    void FreeListAllocator::ReleaseMemory()
    {
        UsedMemory = 0;
        NumAllocations = 0;
        
        FreeList = nullptr;
        
        FreeListSize = 0;
        
        _brkp = nullptr;
        _endp = _brkp;
    }
    
    void *FreeListAllocator::Allocate(u64 size, u64 alignment)
    {
        size = ALIGN(size);
        u64 adj_size = HeaderAdjustedSize(size);
        
        // Search for an available header
        Header *header = nullptr;
        if ((header = FindFreeHeader(size)))
        {
            Split(header, size);
            header->Used = 1;
        }
        else {
            // header was not found, request from OS
            if ((_brkp + adj_size <= _endp))
            {
                char *next_addr = _brkp;
                _brkp += adj_size;
                header = (header_t*)next_addr;
                
                header->Size = size;
                header->Used = 1;
                
                header->Size = size;
                header->Used = 1;
                header->Next = nullptr;
                header->Prev = nullptr;
            }
            else {
                printf("Requesting more memory than is available!\n");
            }
        }
        
        if (header)
        {
            NumAllocations++;
            UsedMemory += header->Size;
        }
        
        return (header) ? Header2Mem(header) : nullptr;
    }
    
    void* FreeListAllocator::Reallocate(void *src, u64 size, u64 alignment)
    {
        /*

2 Cases:

1. New size is equal to the old size
2. New Size is larger than the old size
3. New Size is smaller than the old size

*/
        
        header_t *header = (header_t*)Mem2Header(src);
        
        void *result = nullptr;
        
        size = ALIGN(size);
        if (!src)
        {
            result = Allocate(size, alignment);
        }
        else if (header->Size == size) {
            result = src;
        }
        else if (size > header->Size) {
            // Size is greater than the allocation, so we
            // have allocate a new block of memory, copy
            // the old block over, and finally free the old
            // block
            result = Allocate(size, alignment);
            memcpy(result, src, size);
            Free(src);
        }
        else {
            // Size is less than the allocation, so
            // we attempt to split the block, adjust
            // size, add new block back to the free list
            // and return adjusted block.
            Split(header, size);
            
            result = Header2Mem(header);
        }
        
        return result;
    }
    
    
    void FreeListAllocator::Free(void * ptr)
    {
        if (!ptr) return;
        
        header_t *header = (header_t*)Mem2Header(ptr);
        
        if (!header->Used) {
            return;
        }
        
        header->Used = 0;
        
        if ((i64)UsedMemory - (i64)header->Size < 0)
        {
            printf("Used memory has an underflow!\n");
        }
        
        UsedMemory -= header->Size;
        NumAllocations--;
        
        // When there is an 8 byte allocation, the total allocated size ends up being
        // 24 bytes, and only 8 bytes are reserved (16 bytes are for the Free List and are
        // only needed when in the Free List). So if this block were to be allocated again,
        // there are actually 16 bytes that are usable. This will adjust the header data size
        // to reflect this extra usable space.
        if (header->Size < LIST_HEADER)
            header->Size = LIST_HEADER;
        
        FreeListAdd(header);
    }
} // mm
