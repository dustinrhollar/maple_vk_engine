
#define BLOCK_SIZE       8
#define HEADER_SIZE      BLOCK_SIZE
#define MIN_HEADER_SIZE  sizeof(void*)
#define LIST_HEADER_SIZE 2*MIN_HEADER_SIZE

#define mem_align(n)            ((n) + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1)
#define header_adjusted_size(n) (MIN_HEADER_SIZE + (((n) >= LIST_HEADER_SIZE) ? (n) : LIST_HEADER_SIZE))
#define header_to_mem(h)        (void*)((char*)(h) + MIN_HEADER_SIZE)
#define mem_to_header(p)        (header_t)((char*)(p) - MIN_HEADER_SIZE)

typedef struct header
{
    u64 Size:63;
    u64 Used:1;
    
    header_t Next;
    header_t Prev;
} header;

file_internal header_t memory_find_free_header(header_t* FreeList, u64 Size);
file_internal void memory_free_list_add(header_t *FreeList, header_t Header);
file_internal void memory_free_list_remove(header_t* FreeList, header_t HeaderToRemove);
file_internal header_t memory_block_split(header_t* FreeList, header_t Header, u64 Size);
file_internal void memory_block_coalesce(header_t LeftHeader, header_t RightHeader);

void memory_init(memory *Memory, u64 Size, void *Ptr)
{
    assert(Size % BLOCK_SIZE == 0);
    Memory->Size = Size;
    
    if (!Ptr)
    {
        Memory->Size = 0;
    }
    else
    {
        Memory->Start          = Ptr;
        Memory->Brkp           = Memory->Start;
        Memory->FreeList       = NULL;
        Memory->UsedMemory     = 0;
        Memory->NumAllocations = 0;
    }
}

void memory_free(memory *Memory)
{
    if (Memory->UsedMemory != 0 || Memory->NumAllocations != 0)
    {
        printf("Freeing Free List allocator, but not all memory has been freed. There are still %lld allocations with %lld used memory.\n",
               Memory->NumAllocations, Memory->UsedMemory);
    }
    
    Memory->Start          = NULL;
    Memory->Brkp           = NULL;
    Memory->FreeList       = NULL;
    Memory->Size           = 0;
    Memory->UsedMemory     = 0;
    Memory->NumAllocations = 0;
}

file_internal header_t memory_find_free_header(header_t* FreeList, u64 Size)
{
    header_t Iter = *FreeList;
    
    // Simple first-fit search
    while (Iter != NULL)
    {
        if (!Iter->Used && Size <= Iter->Size)
        {
            break;
        }
        
        Iter = Iter->Next;
    }
    
    if (Iter)
    {
        memory_free_list_remove(FreeList, Iter);
    }
    
    return Iter;
}

file_internal void memory_block_coalesce(header_t LeftHeader, header_t RightHeader)
{
    if (!LeftHeader || !RightHeader)
        return;
    
    // i arbitrarily decided that h1 will be the header
    // that is kept.
    if (LeftHeader < RightHeader)
    {
        LeftHeader->Next = RightHeader->Next;
        if (RightHeader->Next) RightHeader->Next->Prev = LeftHeader;
    }
    else
    {
        LeftHeader->Prev = RightHeader->Prev;
        if (RightHeader->Prev) RightHeader->Prev->Next = LeftHeader;
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
    LeftHeader->Size += header_adjusted_size(RightHeader->Size);
}

file_internal void memory_free_list_add(header_t *FreeList, header_t Header)
{
    Header->Next = NULL;
    Header->Prev = NULL;
    
    // init the list
    if (!(*FreeList))
    {
        *FreeList = Header;
        (*FreeList)->Prev = NULL;
        (*FreeList)->Next = NULL;
        
        return;
    }
    
    header_t Iter = *FreeList;
    
    // add to the front
    if (Header < *FreeList)
    {
        Header->Next      = *FreeList;
        (*FreeList)->Prev = Header;
        *FreeList         = Header;
    }
    else
    {
        while (Iter->Next && Header > Iter->Next)
        {
            // NOTE(Dustin): I am not sure about the relevency of this branch...
            // I think I was originally trying to detect a duplicate in the list.
            if (Iter == Header || Iter->Next == Header)
            {
                return;
            }
            
            Iter = Iter->Next;
        }
        
        // insert the header into the list
        Header->Prev = Iter;
        Header->Next = Iter->Next;
        
        if (Iter->Next)
        {
            Iter->Next->Prev = Header;
        }
        
        Iter->Next = Header;
    }
    
    // If the new node is besides already existing nodes, merge them together
    Iter = Header;
    
    if (Iter->Next)
    {
        u64 Adj = header_adjusted_size(Iter->Size);
        if (((char*)Iter + Adj) == (char*)Iter->Next)
        {
            memory_block_coalesce(Iter, Iter->Next);
        }
    }
    
    if (Iter->Prev)
    {
        u64 Adj = header_adjusted_size(Iter->Prev->Size);
        if (((char*)Iter - Adj) == (char*)Iter->Prev)
        {
            memory_block_coalesce(Iter->Prev, Iter);
        }
    }
}


file_internal void memory_free_list_remove(header_t* FreeList, header_t HeaderToRemove)
{
    // Do not need to check FreeList because Alloc only calls memory_find_free_header
    // if the FreeList exists
    // if (!(*FreeList) || header == NULL) return;
    
    if (*FreeList == HeaderToRemove)
    {
        *FreeList = HeaderToRemove->Next;
        if (*FreeList) (*FreeList)->Prev = NULL;
    }
    
    // Remove it from the free list
    if (HeaderToRemove->Prev != NULL)
    {
        HeaderToRemove->Prev->Next = HeaderToRemove->Next;
    }
    
    if (HeaderToRemove->Next != NULL)
    {
        HeaderToRemove->Next->Prev = HeaderToRemove->Prev;
    }
    
    HeaderToRemove->Prev = NULL;
    HeaderToRemove->Next = NULL;
}

file_internal header_t memory_block_split(header_t* FreeList, header_t Header, u64 Size)
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
      ------------------------------------------
      | 8b | size | 8b | leftover at least 16b |
      ------------------------------------------
    
    */
    
    // size required to form a new block with the requested size
    u64 ReqSize        = header_adjusted_size(Size);
    u64 HeaderDataSize = Header->Size;
    
    // Requested memory is over budget
    if (ReqSize >= HeaderDataSize) return Header;
    
    u64 Leftover = HeaderDataSize - Size;
    // is there enough space for the split? Need: 8 byte core header + 16 bytes for the links
    if (Leftover < header_adjusted_size(BLOCK_SIZE))
    {
        return Header;
    }
    
    Header->Size = Size;
    
    header_t SplitHeader = (header_t)((char*)Header + ReqSize);
    // leftover represents the TOTAL space leftover. Need to account for
    // the reserved flags space, so subtract 8bytes
    SplitHeader->Size = Leftover - BLOCK_SIZE;
    SplitHeader->Used = 0;
    
    // Add the split header to the free list for future use
    memory_free_list_add(FreeList, SplitHeader);
    
    return Header;
}

void* memory_alloc(memory *Memory, u64 Size)
{
    if (Size == 0) return NULL;
    
    void *Result = NULL;
    
    Size = mem_align(Size);
    u64 AdjSize = header_adjusted_size(Size);
    
    // Search for an available header
    header_t Header = NULL;
    if (Memory->FreeList && (Header = memory_find_free_header(&Memory->FreeList, Size)))
    {
        memory_block_split(&Memory->FreeList, Header, Size);
        Header->Used = 1;
        
        Memory->NumAllocations++;
        Memory->UsedMemory += Header->Size;
        
        Result = header_to_mem(Header);
    }
    else {
        // header was not found, request from the heap
        if (((char*)Memory->Brkp + AdjSize) <= ((char*)Memory->Start + Memory->Size))
        {
            char *NextAddr = (char*)Memory->Brkp;
            Memory->Brkp = (char*)Memory->Brkp + AdjSize;
            Header = (header_t)NextAddr;
            
            Header->Size = Size;
            Header->Used = 1;
            Header->Next = NULL;
            Header->Prev = NULL;
            
            Memory->NumAllocations++;
            Memory->UsedMemory += Header->Size;
            
            Result = header_to_mem(Header);
        }
        else {
            // TODO(Dustin): Log
            printf("Requesting more memory than is available!\n");
        }
    }
    
    return Result;
}

void* memory_realloc(memory *Memory, void *Ptr, u64 Size)
{
    header_t Header = (header_t)mem_to_header(Ptr);
    
    void *Result = NULL;
    
    Size = mem_align(Size);
    if (!Ptr)
    {
        Result = memory_alloc(Memory, Size);
    }
    else if (Header->Size == Size)
    {
        Result = Ptr;
    }
    else if (Size > Header->Size)
    {
        // Size is greater than the allocation, so we
        // have allocate a new block of memory, copy
        // the old block over, and finally free the old
        // block
        Result = memory_alloc(Memory, Size);
        memcpy(Result, Ptr, Size);
        memory_release(Memory, Ptr);
    }
    else
    {
        // Size is less than the allocation, so
        // we attempt to split the block, adjust
        // size, add new block back to the free list
        // and return adjusted block.
        memory_block_split(&Memory->FreeList, Header, Size);
        
        Result = header_to_mem(Header);
    }
    
    return Result;
}

void memory_release(memory *Memory, void *Ptr)
{
    if (!Ptr) return;
    
    header_t Header = (header_t)mem_to_header(Ptr);
    
    if (!Header->Used)
    {
        return;
    }
    
    Header->Used = 0;
    
    if ((i64)Memory->UsedMemory - (i64)Header->Size < 0)
    {
        printf("Used memory has an underflow!\n");
    }
    
    Memory->UsedMemory -= Header->Size;
    Memory->NumAllocations--;
    
    // When there is an 8 byte allocation, the total allocated size ends up being
    // 24 bytes, and only 8 bytes are reserved (16 bytes are for the Free List and are
    // only needed when in the Free List). So if this block were to be allocated again,
    // there are actually 16 bytes that are usable. This will adjust the header data size
    // to reflect this extra usable space.
    if (Header->Size < LIST_HEADER_SIZE)
        Header->Size = LIST_HEADER_SIZE;
    
    memory_free_list_add(&Memory->FreeList, Header);
}

#undef mem_to_header
#undef header_to_mem
#undef header_adjusted_size
#undef mem_align
#undef LIST_HEADER_SIZE
#undef MIN_LINK_SIZE
#undef HEADER_SIZE
#undef BLOCK_SIZE
