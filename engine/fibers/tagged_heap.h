#ifndef TAGGED_HEAP_H
#define TAGGED_HEAP_H

struct binary_tag_tree_node;
struct tagged_heap_block;

struct bit_list
{
    u64 *Bits;
    u8  NumInts; // number of ints in the list, the number of bits is: Size * 64
};

struct tagged_heap_tag
{
    u64 Id;
    bit_list AllocationsMask; // bitlist of all allocations in the heap for THIS tag
};

struct tagged_heap
{
    void                 *Start; // start of the memory allcoations
    
    u64                   Size;       // total size of the allocation
    u64                   BlockSize;  // size per block
    
    bit_list              AllocationList; // bitlist of all allocations in the heap
    
    // Tags are stored in a binary tree for fast lookups
    binary_tag_tree_node *Root;
};

// When a new tag allocation request is requested a block of memory
// is returned. This block of memory acts as a linear allocation.
// Free is an invalid call on an individual tagged_heap_block, instead
// Free must be called on the tag, which will free all blocks associated
// with that tag.
struct tagged_heap_block
{
    char        *Start; // start of the block memory
    char        *End;   // end of the block memory
    char        *Brkp;  // current location in the block memory
    
    // NOTE(Dustin): Need this back pointer to the TaggedHeap?
    tagged_heap *TaggedHeap;
};

void InitTaggedHeap(tagged_heap *TaggedHeap, u64 AllocationSize, u64 BlockSize);
void FreeTaggedHeap(tagged_heap *TaggedHeap);

// Returns a block allocation associated with the tag
// tagged_heap_block doesn't actually own any of its pointers, returing
// on the stack should be fine.
tagged_heap_block TaggedHeapRequestAllocation(tagged_heap *TaggedHeap, u64 Tag);
// Frees allocations associated with a tag.
void TaggedHeapReleaseAllocation(tagged_heap *TaggedHeap, u64 Tag); // releases all allocations for the tag!

// Will attempt to allocate from the provided block, if there is not
// enough space, nullptr is returned. Otherwise a pointer to the new
// allocation is returned.
void* TaggedHeapBlockAlloc(tagged_heap_block *Block, u64 Size);

#endif //TAGGED_HEAP_H
