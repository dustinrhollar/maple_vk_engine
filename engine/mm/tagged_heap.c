
#define BIT_TOGGLE(n, b, v) ((n) = ((n) & ~(1ULL << (b))) | ((v) << (b)))
#define BIT_TOGGLE_0(n ,b) BIT_TOGGLE(n, b, 0)
#define BIT_TOGGLE_1(n, b) BIT_TOGGLE(n, b, 1)
#define BITMASK_CLEAR(x,m) ((x) &=(~(m)))

file_internal void BitListInit(bit_list *Bitlist, free_allocator *Allocator, u8 NumInts)
{
    // Total number of u64s required by the bits lis
    Bitlist->NumInts = NumInts;
    Bitlist->Bits    = (u64*)FreeListAllocatorAlloc(Allocator, Bitlist->NumInts * sizeof(u64));
    for (u64 Bitset = 0; Bitset < Bitlist->NumInts; ++Bitset)
        Bitlist->Bits[Bitset] = 0;
}

file_internal void BitListFree(bit_list *Bitlist, free_allocator *Allocator)
{
    FreeListAllocatorAllocFree(Allocator, Bitlist->Bits);
    Bitlist->Bits    = NULL;
    Bitlist->NumInts = 0;
}

file_internal void BitListReset(bit_list *Bitlist)
{
    for (u64 Bitset = 0; Bitset < Bitlist->NumInts; ++Bitset)
        Bitlist->Bits[Bitset] = 0;
}

file_internal void BitListMove(bit_list *Dst, bit_list *Src)
{
    for (u64 Bitset = 0; Bitset < Dst->NumInts; ++Bitset)
        Dst->Bits[Bitset] = Src->Bits[Bitset];
}

file_internal void ClearBitListWithMask(bit_list *Source, bit_list *Mask)
{
    assert(Source->NumInts == Mask->NumInts);

    for (u8 Bitset = 0; Bitset < Mask->NumInts; ++Bitset)
        BITMASK_CLEAR(Source->Bits[Bitset], Mask->Bits[Bitset]);

}

void TaggedHeapInit(tagged_heap *TaggedHeap, free_allocator *Allocator, u64 AllocationSize, u32 BlockSize, u32 MaxActiveTags)
{
    TaggedHeap->Start     = (char*)FreeListAllocatorAlloc(Allocator, AllocationSize);
    TaggedHeap->Size      = AllocationSize;
    TaggedHeap->BlockSize = BlockSize;

    u64 NumBlocks = AllocationSize / BlockSize;

    // Init the bit list of block allocations...The number of required bit is NumBlocks
    u32 NumInts = NumBlocks / 64;
    if ((NumBlocks % 64) != 0) NumInts++;
    assert(NumInts < 255 && "Tagged heap bit list requires more than 255 ints!");

    BitListInit(&TaggedHeap->AllocationList, Allocator, (u8)(NumInts));

    TaggedHeap->MaxActiveTags = MaxActiveTags;
    TaggedHeap->ActiveTagsCount = 0;
    TaggedHeap->ActiveTags = (heap_tag_t*)FreeListAllocatorAlloc(Allocator, sizeof(tagged_heap_tag) * TaggedHeap->MaxActiveTags);

    for (u32 i = 0; i < TaggedHeap->MaxActiveTags; ++i)
    {
        BitListInit(&TaggedHeap->ActiveTags[i].AllocationsMask, Allocator, (u8)NumInts);
    }
}

void TaggedHeapFree(tagged_heap *TaggedHeap, free_allocator *Allocator)
{
    FreeListAllocatorAllocFree(Allocator, TaggedHeap->Start);
    BitListFree(&TaggedHeap->AllocationList, Allocator);

    for (u32 i = 0; i < TaggedHeap->MaxActiveTags; ++i)
    {
        BitListFree(&TaggedHeap->ActiveTags[i].AllocationsMask, Allocator);
    }
    FreeListAllocatorAllocFree(Allocator, TaggedHeap->ActiveTags);

    TaggedHeap->Start           = NULL;
    TaggedHeap->ActiveTags      = NULL;
    TaggedHeap->Size            = 0;
    TaggedHeap->BlockSize       = 0;
    TaggedHeap->ActiveTagsCount = 0;
    TaggedHeap->MaxActiveTags   = 0;
}

bool CompareTags(tag_id_t Lhs, tag_id_t Rhs)
{
    return (Lhs.Frame == Rhs.Frame) && (Lhs.Id == Rhs.Id);
}

// Returns a block allocation associated with the tag
// tagged_heap_block doesn't actually own any of its pointers, returing
// on the stack should be fine.
tagged_heap_block TaggedHeapRequestAllocation(tagged_heap *TaggedHeap, tag_id_t Tag)
{
    tagged_heap_block Result = {0};

    // Retrieve the Tag Node in the heap
    // NOTE(Dustin): Currently requires 2 tree traversals in worst case...
    // Maybe try have Insert return an existing node if the tag already exists
    // in the tree...
    //binary_tag_tree_node *TaggedNode = BinaryTagTreeFind(TaggedHeap->Root, Tag);

    bool TagFound = false;
    heap_tag_t *HeapTag;

    // Check if the tag currently exists in the heap
    for (u32 i = 0; i < TaggedHeap->ActiveTagsCount; ++i)
    {
        if (CompareTags(Tag, TaggedHeap->ActiveTags[i].Tag))
        {
            TagFound = true;
            HeapTag = &TaggedHeap->ActiveTags[i];
            break;
        }
    }

    if (!TagFound)
    {
        if (TaggedHeap->ActiveTagsCount + 1 > TaggedHeap->MaxActiveTags)
        {
            printf("Current Active Tag Count %d. Max active tags %d\n", TaggedHeap->ActiveTagsCount, TaggedHeap->MaxActiveTags);
            printf("ERROR: No more available tagged heaps! Free an existing tag to allocate more!\n");
            return Result;
        }

        HeapTag = &TaggedHeap->ActiveTags[TaggedHeap->ActiveTagsCount];
        HeapTag->Tag = Tag;

        TaggedHeap->ActiveTagsCount++;
    }

    u64 Bitset;
    for (u32 i = 0; i < TaggedHeap->AllocationList.NumInts; ++i)
    {
        // Negate the particular bitset. Ctz will find the first
        // 1 starting from the the least significant digit. However,
        // for the purposes of this bitlist, a 1 means an allocation,
        // but we want to find unallocated blocks. Negating the bitset will
        // set all allocated blocks to 0 and unallocated blocks to 1.
        // The found idx is the index of the block we want to retrieve
        // for the allocation.
        Bitset = ~TaggedHeap->AllocationList.Bits[i];

        // Ctz() is undefined when the number == 0
        // If the nagated bitset == 0, then there are
        // no available allocations in the block.
        // If there are no allocations, then go to the
        // next bitset.
        u32 Idx = 0;
        u32 Bit = 0;
        if (Bitset)
            Bit = PlatformCtzl(Bitset);
        else
            continue;
        Idx = i * 64 + Bit;

        // Init the allocation info
        char *BlockStart = (char*)TaggedHeap->Start + Idx * TaggedHeap->BlockSize;

        Result.Start      = BlockStart;
        Result.End        = BlockStart + TaggedHeap->BlockSize;
        Result.Brkp       = Result.Start;
        Result.TaggedHeap = TaggedHeap;

        // Mark the allocation in the TaggedHeap
        BIT_TOGGLE_1(TaggedHeap->AllocationList.Bits[i], Bit);
        BIT_TOGGLE_1(HeapTag->AllocationsMask.Bits[i], Bit);

        break;
    }

    if (Result.Start == NULL)
        printf("ERROR: Tagged Heap could not find an available block to allocate!\n");

    return Result;
}


// Frees allocations associated with a tag.
void TaggedHeapReleaseAllocation(tagged_heap *TaggedHeap, tag_id_t Tag)
{
    // Check if the tag currently exists in the heap
    for (u32 i = 0; i < TaggedHeap->ActiveTagsCount; ++i)
    {
        if (CompareTags(Tag, TaggedHeap->ActiveTags[i].Tag))
        {
            ClearBitListWithMask(&TaggedHeap->AllocationList, &TaggedHeap->ActiveTags[i].AllocationsMask);

            // TODO Remove from the list
            BitListMove(&TaggedHeap->ActiveTags[i].AllocationsMask,
                        &TaggedHeap->ActiveTags[TaggedHeap->ActiveTagsCount-1].AllocationsMask);
            BitListReset(&TaggedHeap->ActiveTags[TaggedHeap->ActiveTagsCount-1].AllocationsMask);

            TaggedHeap->ActiveTagsCount--;
            break;
        }
    }
}

// Will attempt to allocate from the provided block, if there is not
// enough space, nullptr is returned. Otherwise a pointer to the new
// allocation is returned.
void* TaggedHeapBlockAlloc(tag_block_t Block, u64 Size)
{
    void *Result = NULL;

    // Align Size to be on an 8byte boundary. Yea, this is hardcoded,
    // but I dont care about supporting 32bit machines
    Size = (Size + 7) & ~(7); // 0b1000 = ~7
    assert(Size % 8 == 0 && "Hardcoded alignment failed in Tagged Heap Block Alloc!");

    if (Block->Brkp + Size <= Block->End)
    {
        Result = Block->Brkp;
        Block->Brkp += Size;
    }

    return Result;
}

#undef BIT_TOGGLE
#undef BIT_TOGGLE_0
#undef BIT_TOGGLE_1
#undef BITMASK_CLEAR
