#define BIT(x) (1<<(x))

#define BIT_TOGGLE(n, b, v) ((n) = ((n) & ~(1ULL << (b))) | ((v) << (b)))
#define BIT_TOGGLE_0(n ,b) BIT_TOGGLE(n, b, 0)
#define BIT_TOGGLE_1(n, b) BIT_TOGGLE(n, b, 1)

#define BITMASK_CLEAR(x,m) ((x) &=(~(m)))

struct binary_tag_tree_node
{
    binary_tag_tree_node *Left;
    binary_tag_tree_node *Right;
    
    tagged_heap_tag Tag;
};

file_internal void InitBitList(bit_list *Bitlist, u8 NumInts);
file_internal void FreeBitList(bit_list *Bitlist);
file_internal void ClearBitListWithMask(bit_list *Source, bit_list *Mask);
file_internal void BinaryTagTreeFree(binary_tag_tree_node *Root);

// Returns a pointer to the newly added Node. The Bitlist is not yet
// initialized for this node
file_internal binary_tag_tree_node* BinaryTagTreeInsert(binary_tag_tree_node **Root, u64 TagId);
file_internal tagged_heap_tag BinaryTagTreeRemove(binary_tag_tree_node **Root, u64 TagId);
file_internal binary_tag_tree_node* BinaryTagTreeFind(binary_tag_tree_node **Root, u64 TagId);

file_internal void InitBitList(bit_list *Bitlist, u8 NumInts)
{
    // Total number of u64s required by the bits lis
    Bitlist->NumInts = NumInts;
    Bitlist->Bits    = palloc<u64>(Bitlist->NumInts);
    for (u64 Bitset = 0; Bitset < Bitlist->NumInts; ++Bitset)
        Bitlist->Bits[Bitset] = 0;
}

file_internal void FreeBitList(bit_list *Bitlist)
{
    pfree(Bitlist->Bits);
    Bitlist->NumInts = 0;
}

file_internal void ClearBitListWithMask(bit_list *Source, bit_list *Mask)
{
    assert(Source->NumInts == Mask->NumInts);
    
    for (u8 Bitset = 0; Bitset < Mask->NumInts; ++Bitset)
        BITMASK_CLEAR(Source->Bits[Bitset], Mask->Bits[Bitset]);
    
}

// AllocationSize: Total size in bytes of the heap
// BlockSize: Size of each block that can be allocated
void InitTaggedHeap(tagged_heap *TaggedHeap, u64 AllocationSize, u64 BlockSize)
{
    TaggedHeap->Start = palloc(AllocationSize);
    TaggedHeap->Size  = AllocationSize;
    TaggedHeap->BlockSize = BlockSize;
    
    u64 NumBlocks = AllocationSize / BlockSize;
    
    // Init the bit list of block allocations...The number of required bit is NumBlocks
    r32 NumInts = ceilf(static_cast<r32>(NumBlocks) / 64.0f);
    assert(NumInts < 255.0f && "Tagged heap bit list requires more than 255 ints!");
    
    InitBitList(&TaggedHeap->AllocationList, static_cast<u8>(NumInts));
    
    // Tags are stored in a binary tree for fast lookups
    TaggedHeap->Root = nullptr;
}

void FreeTaggedHeap(tagged_heap *TaggedHeap)
{
    FreeBitList(&TaggedHeap->AllocationList);
    BinaryTagTreeFree(TaggedHeap->Root);
    pfree(TaggedHeap->Start);
    
    TaggedHeap->Size           = 0;
    TaggedHeap->BlockSize      = 0;
    TaggedHeap->AllocationList = {0};
    TaggedHeap->Root           = nullptr;
}

// NOTE(Dustin): Not yet implemented because I think this should not have to be
// called! If a user has cleaned their memory appropriately, then the Root node
// should be nullptr!
file_internal void BinaryTagTreeFree(binary_tag_tree_node *Root)
{
    if (Root)
        mprinte("Binary Tag Tree Free was called, and the Root was not yet null! Tags have not been freed!");
}

file_internal binary_tag_tree_node* BinaryTagTreeFind(binary_tag_tree_node *Root, u64 TagId)
{
    binary_tag_tree_node *Result = nullptr;
    binary_tag_tree_node *Iter = Root;
    
    while (Iter)
    {
        if (TagId == Iter->Tag.Id)
        {
            Result = Iter;
            break;
        }
        else if (TagId < Iter->Tag.Id)
        { // left node exists, traverse to that node
            Iter = Iter->Left;
        }
        else
        { // Right node exists, let's go to that one
            Iter = Iter->Right;
        }
    }
    
    return Result;
}

file_internal binary_tag_tree_node* BinaryTagTreeInsert(binary_tag_tree_node **Root, u64 TagId)
{
    binary_tag_tree_node *Result = nullptr;
    binary_tag_tree_node *Iter = *Root;
    if (Iter)
    { // Make sure the root actually exists
        while (true)
        {
            if (TagId < Iter->Tag.Id && !Iter->Left)
            { // Needs to be inserted to left node
                Iter->Left = palloc<binary_tag_tree_node>(1);
                Iter->Left->Tag.Id = TagId;;
                Iter->Left->Tag.AllocationsMask = {};
                
                Result = Iter->Left;
                break;
            }
            else if (TagId < Iter->Tag.Id)
            { // left node exists, traverse to that node
                Iter = Iter->Left;
            }
            else if (TagId >= Iter->Tag.Id && !Iter->Right)
            { // Needs to be inserted to left node
                Iter->Right = palloc<binary_tag_tree_node>(1);
                Iter->Right->Tag.Id = TagId;
                Iter->Right->Tag.AllocationsMask = {};
                
                Result = Iter->Right;
                break;
            }
            else
            { // Right node exists, let's go to that one
                Iter = Iter->Right;
            }
        }
    }
    else
    {
        *Root = palloc<binary_tag_tree_node>(1);
        (*Root)->Left   = nullptr;
        (*Root)->Right  = nullptr;
        (*Root)->Tag.Id = TagId;;
        Result = *Root;
    }
    
    return Result;
}

file_internal tagged_heap_tag BinaryTagTreeRemove(binary_tag_tree_node **Root, u64 TagId)
{
    // Cases:
    // - Node is a leaf
    // - Node has one child
    // - Node has two children
    
    // Special Case:
    // - Node is the Root
    tagged_heap_tag Result = {};
    
    //~ Find the node and its parent in the tree
    binary_tag_tree_node *NodeToRemove       = nullptr;
    binary_tag_tree_node *NodeToRemoveParent = nullptr;
    {
        binary_tag_tree_node *Iter = *Root;
        while (Iter)
        {
            if (TagId == Iter->Tag.Id)
            {
                NodeToRemove = Iter;
                break;
            }
            else if (TagId < Iter->Tag.Id)
            { // left node exists, traverse to that node
                NodeToRemoveParent = Iter;
                Iter = Iter->Left;
            }
            else
            { // Right node exists, let's go to that one
                NodeToRemoveParent = Iter;
                Iter = Iter->Right;
            }
        }
    }
    
    //~ Remove the node
    if (!NodeToRemove)
    {
        mprinte("Attempted to remove Tag %d from the Tag Tree, but it did not exist!\n", TagId);
    }
    else
    {
        Result = NodeToRemove->Tag;
        
        if (!NodeToRemove->Left && !NodeToRemove->Right)
        { // no children
            if (!NodeToRemoveParent)
            { // node being removed is the root node
                assert((*Root)->Tag.Id == TagId && "Found a null parent when removing from the Tag Tree, but the node was not the root!"); // make doubly sure this is the case
                
                *Root = nullptr;
            }
            else
            {
                if (NodeToRemoveParent->Left && NodeToRemoveParent->Left->Tag.Id == TagId)
                { // node being removed is the left node
                    NodeToRemoveParent->Left = nullptr;
                }
                else
                { // node if the right node
                    NodeToRemoveParent->Right = nullptr;
                }
            }
            
            pfree(NodeToRemove);
        }
        else if (NodeToRemove->Left && NodeToRemove->Right)
        { // both children exist
            // Find the in order successor of the node
            binary_tag_tree_node *Successor       = NodeToRemove->Right;
            binary_tag_tree_node *SuccessorParent = NodeToRemove;
            
            while (Successor->Left)
            {
                SuccessorParent = Successor;
                Successor = Successor->Left;
            }
            
            SuccessorParent->Left = nullptr;
            NodeToRemove->Tag = Successor->Tag;
            pfree(Successor);
        }
        else
        { // one of the children
            if (NodeToRemove->Left)
            {
                binary_tag_tree_node *tmp = NodeToRemove->Left;
                
                NodeToRemove->Tag   = NodeToRemove->Left->Tag;
                NodeToRemove->Right = NodeToRemove->Left->Right;
                NodeToRemove->Left  = NodeToRemove->Left->Left;
                
                pfree(tmp);
            }
            else
            {
                binary_tag_tree_node *tmp = NodeToRemove->Right;
                
                NodeToRemove->Tag   = NodeToRemove->Right->Tag;
                NodeToRemove->Left  = NodeToRemove->Right->Left;
                NodeToRemove->Right = NodeToRemove->Right->Right;
                
                pfree(tmp);
            }
        }
    }
    
    return Result;
}

// Returns a block allocation associated with the tag
tagged_heap_block TaggedHeapRequestAllocation(tagged_heap *TaggedHeap, u64 Tag)
{
    tagged_heap_block Result = {};
    
    // Retrieve the Tag Node in the heap
    // NOTE(Dustin): Currently requires 2 tree traversals in worst case...
    // Maybe try have Insert return an existing node if the tag already exists
    // in the tree...
    binary_tag_tree_node *TaggedNode = BinaryTagTreeFind(TaggedHeap->Root, Tag);
    if (!TaggedNode)
    {
        TaggedNode = BinaryTagTreeInsert(&TaggedHeap->Root, Tag);
        InitBitList(&TaggedNode->Tag.AllocationsMask, TaggedHeap->AllocationList.NumInts);
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
            Bit = PlatformCtz(Bitset);
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
        BIT_TOGGLE_1(TaggedNode->Tag.AllocationsMask.Bits[i], Bit);
        
        break;
    }
    
    if (Result.Start == nullptr)
        mprinte("Tagged Heap could not find an available block to allocate!\n");
    
    return Result;
}

// Frees allocations associated with a tag.
// releases all allocations for the tag!
void TaggedHeapReleaseAllocation(tagged_heap *TaggedHeap, u64 Tag)
{
    // Remove the tag  from the binary tree
    tagged_heap_tag RemovedTag = BinaryTagTreeRemove(&TaggedHeap->Root, Tag);
    ClearBitListWithMask(&TaggedHeap->AllocationList, &RemovedTag.AllocationsMask);
    FreeBitList(&RemovedTag.AllocationsMask);
}

// Will attempt to allocate from the provided block, if there is not
// enough space, nullptr is returned. Otherwise a pointer to the new
// allocation is returned..
void* TaggedHeapBlockAlloc(tagged_heap_block *Block, u64 Size)
{
    void *Result = nullptr;
    
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

#undef BIT
#undef BIT_TOGGLE
#undef BIT_TOGGLE_0
#undef BIT_TOGGLE_1
#undef BITMASK_CLEAR