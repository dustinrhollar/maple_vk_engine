#ifndef MAPLE_MM_TAGGED_HEAP_H
#define MAPLE_MM_TAGGED_HEAP_H

#define TAG_ID_PLATFORM 0
#define TAG_ID_FILE     1
#define TAG_ID_GAME     2
#define TAG_ID_RENDER   3
#define TAG_ID_GPU      4
#define TAG_ID_ASSET    5

struct binary_tag_tree_node;

typedef struct
{
    u64 *Bits;
    u8   NumInts; // number of ints in the list, the number of bits is: Size * 64
} bit_list;

typedef struct
{
    i64 Frame:48;  // current frame number
    i64 Id:8;      // randomely generated id
    i64 Pad:8;     // reserved space for future use
} tag_id_t;

typedef struct
{
    // Id
    tag_id_t Tag;
    
    // bitlist of all allocations in the heap for THIS tag
    bit_list AllocationsMask;
} tagged_heap_tag;

typedef struct
{
    void                 *Start; // start of the memory allcoations
    
    u64                   Size;       // total size of the allocation
    u64                   BlockSize;  // size per block
    
    bit_list              AllocationList; // bitlist of all allocations in the heap
    
    // This array keeps track of all active tags and their allocated tagged blocks
    tagged_heap_tag *ActiveTags;
    u32              ActiveTagsCount;
    u32              MaxActiveTags;
    // Tags are stored in a binary tree for fast lookups
    //binary_tag_tree_node *Root;
} tagged_heap;

// When a new tag allocation request is requested a block of memory
// is returned. This block of memory acts as a linear allocation.
// Free is an invalid call on an individual tagged_heap_block, instead
// Free must be called on the tag, which will free all blocks associated
// with that tag.
typedef struct
{
    char        *Start; // start of the block memory
    char        *End;   // end of the block memory
    char        *Brkp;  // current location in the block memory
    
    // NOTE(Dustin): Need this back pointer to the TaggedHeap?
    tagged_heap *TaggedHeap;
} tagged_heap_block;

typedef tagged_heap_tag    heap_tag_t;
typedef tagged_heap_block* tag_block_t;


void TaggedHeapInit(tagged_heap *TaggedHeap, free_allocator *Allocator, u64 AllocationSize, u32 BlockSize, u32 MaxActiveTags);
void TaggedHeapFree(tagged_heap *TaggedHeap, free_allocator *Allocator);

bool CompareTags(tag_id_t Lhs, tag_id_t Rhs);

// Returns a block allocation associated with the tag
// tagged_heap_block doesn't actually own any of its pointers, returing
// on the stack should be fine.
tagged_heap_block TaggedHeapRequestAllocation(tagged_heap *TaggedHeap, tag_id_t Tag);
// Frees allocations associated with a tag
// releases all allocations for the tag!.
void TaggedHeapReleaseAllocation(tagged_heap *TaggedHeap, tag_id_t Tag);


// Will attempt to allocate from the provided block, if there is not
// enough space, nullptr is returned. Otherwise a pointer to the new
// allocation is returned.
void* TaggedHeapBlockAlloc(tag_block_t Block, u64 Size);

#endif //MAPLE_MM_TAGGED_HEAP_H
