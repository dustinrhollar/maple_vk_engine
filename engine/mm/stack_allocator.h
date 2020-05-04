#ifndef JACKAL_MM_STACK_ALLOCATOR_H
#define JACKAL_MM_STACK_ALLOCATOR_H

/*

A stack allocator  works similar to a stack data structure. This class
maintains 3 pointers:
1. Pointer to the start of the stack.
2. Pointer to the top of the stack.
3. Pointer to the last allocation.

For each allocation, a header is allocated. A header contains
the following information:
1. Adjustment for the allocation.
2. Pointer to the last allocation.

The total allocated memory for an allocation could look like:

------------------------------------------------------
| M Alignment | Header 16 bytes | Allocation N bytes |
------------------------------------------------------

Where possible, the header and the alignment are combined in
order to reduce the memory overhead.

It is expected that when freeing allocations, allocations are freed
in the opposite order they were allocated. For example, if the stack looked
like:
  Top
              | C |
              | B |
              | A |
              Bottom

It is expected that Allocation C would be freed before B and A. However,
if the user wishes to Free Allocation A without having to explicitly Free
Allocations B and C, the user is able to do so. But by calling Free(A) with
Allocations B and C still on the stack will result in allocations B and C
being freed with A.

This StackAllocator does not support out-of-order frees, so if an out-of-order
free is called, then all elements from the top of the stack to the requested free
will be freed.

*/

namespace mm {
    
    class StackAllocator : public Allocator
    {
        public:
        
        StackAllocator(size_t size, void *start);
        ~StackAllocator();
        
        // Copy and Move not allowed for an allocator
        StackAllocator(StackAllocator & other) = delete;
        StackAllocator(StackAllocator && other) = delete;
        
        StackAllocator& operator=(StackAllocator & other)  = delete;
        StackAllocator& operator=(StackAllocator && other) = delete;
        
        virtual void * Allocate(size_t size, size_t alignment)             override;
        virtual void* Reallocate(void *src, size_t size, size_t alignment) override;
        virtual void Free(void * ptr)                                      override;
        
        private:
        
        struct Header
        {
            u8 Alignment;
            void * PrevAddr;
        };
        
        void *PrevPos; // start of the previous allocation on the stack
        void *CurPos; // current position on the stack
        
        inline Header* Mem2Header(void *mem) {return (Header*)((char*)mem-sizeof(Header));}
        inline void*   Header2Mem(Header *header) {return (void*)((char*)header+sizeof(header));}
    };
    
} // mm

#endif //JACKAL_MM_STACK_ALLOCATOR_H
