
namespace mm {
    
#define BLOCK_SIZE 8
#define HEADER_SIZE      BLOCK_SIZE
#define MIN_LINK_SIZE    sizeof(void*)
#define MIN_HEADER_SIZE  sizeof(size_t)
#define LIST_HEADER      2*MIN_LINK_SIZE
    
    // Align a size request
#define ALIGN(n) (((n) + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1))
    
    // 0 is unused, 1 is used
#define header_toggle_0(b) ((b)->Flags &= ~1L)
#define header_toggle_1(b) ((b)->Flags |= 1L)
#define header_used(b)     (((b)->Flags) & 1L)
    
#define data_size(b)        (((b)->Flags & ~1L) >> 1)
#define data_set_size(b, s) (b)->Flags = (header_used(b) | ((s) << 1))
    
    inline size_t link_adjusted_size(size_t s) {
        size_t result;
        if (s >= LIST_HEADER)
            result = s;
        else
            result = LIST_HEADER;
        
        return result;
    }
    
    inline size_t header_adjusted_size(size_t n)
    {
        size_t result = link_adjusted_size(n);
        return result + MIN_HEADER_SIZE;
    }
    
    void FreeListAllocator::coalesce(header_t *left_header, header_t *right_header)
    {
        //printf("\tCoalesce occuring between %p and %p\n", left_header, right_header);
        
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
        
        size_t h1_size = data_size(left_header);
        size_t h2_size = header_adjusted_size(data_size(right_header));
        
        data_set_size(left_header, h1_size + h2_size);
    }
    
    FreeListAllocator::header_t* FreeListAllocator::split(header_t *header, size_t size)
    {
        //printf("\tSPLIT operation %p\n", header);
        
        // Determine if a header can be split
        /*
     
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
        size_t req_size         = header_adjusted_size(size);
        size_t header_data_size = data_size(header);
        
        // Requested memory is over budget
        if (req_size >= header_data_size) return header;
        
        size_t leftover  = header_data_size - req_size;
        // not enough leftover space for an allocation
        if (leftover < HEADER_SIZE) {
            return header;
        }
        
        data_set_size(header, size); // size being requested
        
        header_t *split_header = (header_t*)((char*)header + req_size);
        // leftover represents the TOTAL space leftover. Need to account for
        // the reserved flags space, so subtract 8bytes
        data_set_size(split_header, leftover - BLOCK_SIZE);
        header_toggle_0(split_header);
        // Add the split header to the free list for future use
        free_list_add(split_header);
        
        //UsedMemory -= leftover;
        
        return header;
    }
    
    inline void FreeListAllocator::free_list_add(header_t *header)
    {
        //printf("\tFreeList add with %p\n", header);
        
        header->Next = nullptr;
        header->Prev = nullptr;
        
        // init the list
        if (FreeList == nullptr)
        {
            //printf("\t\tFreeList header updated from NULL to %p\n", header);
            FreeList = header;
            FreeList->Prev = nullptr;
            FreeList->Next = nullptr;
            
            return;
        }
        
        header_t *iter = FreeList;
        
        // add to the front
        if (header < FreeList)
        {
            //printf("\t\tFreeList header updated from %p to %p\n", FreeList, header);
            
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
            size_t adj = header_adjusted_size(data_size(iter));
            if (((char*)iter + adj) == (char*)iter->Next)
            {
                coalesce(iter, iter->Next);
            }
        }
        
        if (iter->Prev)
        {
            size_t adj = header_adjusted_size(data_size(iter->Prev));
            if (((char*)iter - adj) == (char*)iter->Prev)
            {
                coalesce(iter->Prev, iter);
            }
        }
    }
    
    FreeListAllocator::header_t* FreeListAllocator::FindFreeHeader(size_t size)
    {
        header_t *header = FreeList;
        
        int count = 0;
        while (header != nullptr)
        {
            if (!header_used(header) && size <= data_size(header))
            {
                break;
            }
            
            count ++;
            header = header->Next;
        }
        
        if (header) {
            free_list_remove(header);
        }
        
        return header;
    }
    
    inline void FreeListAllocator::free_list_remove(header_t *header)
    {
        if (FreeList == nullptr || header == nullptr) return;
        
        if (FreeList == header)
        {
            //printf("\t\tFreeList header updated from %p to %p\n", FreeList, header->Next);
            
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
        return (header_t*)((char*)mem-MIN_HEADER_SIZE);
    }
    
    inline void* FreeListAllocator::Header2Mem(header_t *header) {
        return (void*)((char*)header+MIN_HEADER_SIZE);
    }
    
    
    FreeListAllocator::FreeListAllocator(size_t size, void *start)
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
    
    void *FreeListAllocator::Allocate(size_t size, size_t alignment)
    {
        size = ALIGN(size);
        size_t adj_size = header_adjusted_size(size);
        
        // Search for an available header
        Header *header = nullptr;
        if ((header = FindFreeHeader(size)))
        {
            split(header, size);
            header_toggle_1(header);
        }
        else {
            // header was not found, request from OS
            if ((_brkp + adj_size <= _endp))
            {
                char *next_addr = _brkp;
                _brkp += adj_size;
                header = (header_t*)next_addr;
                
                data_set_size(header, size);
                header_toggle_1(header);
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
            UsedMemory += data_size(header);;
        }
        
        return (header) ? Header2Mem(header) : nullptr;
    }
    
    void* FreeListAllocator::Reallocate(void *src, size_t size, size_t alignment)
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
        else if (data_size(header) == size) {
            result = src;
        }
        else if (size > data_size(header)) {
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
            split(header, size);
            
            result = Header2Mem(header);
        }
        
        return result;
    }
    
    
    void FreeListAllocator::Free(void * ptr)
    {
        if (!ptr) return;
        
        header_t *header = (header_t*)Mem2Header(ptr);
        
        if (!header_used(header)) {
            return;
        }
        
        header_toggle_0(header);
        
        if ((i64)UsedMemory - (i64)data_size(header) < 0)
        {
            printf("Used memory has an underflow!\n");
        }
        
        UsedMemory -= data_size(header);
        NumAllocations--;
        
        // When there is an 8 byte allocation, the total allocated size ends up being
        // 24 bytes, and only 8 bytes are reserved (16 bytes are for the Free List and are
        // only needed when in the Free List). So if this block were to be allocated again,
        // there are actually 16 bytes that are usable. This will adjust the header data size
        // to reflect this extra usable space.
        if (data_size(header) < LIST_HEADER)
            data_set_size(header, LIST_HEADER);
        
        free_list_add(header);
    }
} // mm
