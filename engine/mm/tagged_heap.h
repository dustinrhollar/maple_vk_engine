#ifndef TAGGED_HEAP_H
#define TAGGED_HEAP_H

struct bit_list
{
    u64 *Bit;
    u8  Size; // number of ints in the list, the number of bits is: Size * 64
};

struct tagged_heap_tag
{
    u64 Tag;
    bit_list AllocationsMask; // bitlist of all allocations in the heap for THIS tag
};

struct tagged_heap
{
    tagged_heap_block *Start; // start of the memory allcoations
    
    u64 Size;
    bit_list AllocationList; // bitlist of all allocations in the heap
};

void InitTaggedHeap(tagged_heap *TaggedHeap, u64 AllocationSize, u64 BlockSize);
void FreeTaggedHeap(tagged_heap *TaggedHeap);

void TaggedHeapRequestAllocation(u64 Tag, void *Dst, u64 Count);
void TaggedHeapReleaseAllocation(u64 Tag); // releases all allocations for the tag!

#endif //TAGGED_HEAP_H
