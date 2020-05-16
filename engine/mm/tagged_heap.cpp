#define BIT(x) (1<<(x))

#define BIT_TOGGLE(n, b, v) ((n) = ((n) & ~(1UL << (b))) | ((v) << (b)))
#define BIT_TOGGLE_0(n ,b) TOGGLE(n, b, 0)
#define BIT_TOGGLE_1(n, b) TOGGLE(n, b, 1)

#define BITMASK_CLEAR(x,m) ((x) &= (~(m)))


void InitTaggedHeap(tagged_heap *TaggedHeap, u64 AllocationSize, u64 BlockSize)
{
    
}

void FreeTaggedHeap(tagged_heap *TaggedHeap)
{
    
}

void TaggedHeapRequestAllocation(u64 Tag, void *Dst, u64 Count)
{
    
}

void TaggedHeapReleaseAllocation(u64 Tag)
{
    
}
