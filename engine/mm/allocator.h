#ifndef JENGINE_MM_ALLOCATOR_H
#define JENGINE_MM_ALLOCATOR_H

/*

Allocator is the parent class for all allocator-type
objects. It provides an interface for allocating and
 freeing allocations.

The amount of memory and number of allocations are
tracked by Allocator in order to track memory leaks
within the program. When an Allocator is destroyed
and if the used memory and number of allocations
are not zero, an assertion error is thrown.

*/

namespace mm {
    
    
    file_global const u8 DEFAULT_ALIGNMENT = 8;
    
    
    // Parent Class for allocators.
    // Probides the two following functions for
    // child classes:
    // 1. Allocate(size_t size, size_t alignment);
    // 2. Free(void * ptr)
    class Allocator
    {
        public:
        
        Allocator(size_t size, void *start)
            : Size(size)
            , StartAddr(start)
            , UsedMemory(0)
            , NumAllocations(0)
        {
            assert(Size > 0 && "Size of an allocator must be greater than 0.");
        }
        
        virtual ~Allocator()
        {
            if (UsedMemory != 0)
            {
                printf("Used memory is not 0! Used memory is %zd\n", UsedMemory);
            }
            
            if (NumAllocations != 0)
            {
                printf("NumAllocation is not 0! NumAllocations is %zd\n", NumAllocations);
            }
            
            StartAddr = nullptr;
            Size = 0;
        }
        
        // Copy and Move not allowed for an allocator
        Allocator(Allocator & other) = delete;
        Allocator(Allocator && other) = delete;
        
        Allocator& operator=(Allocator & other)  = delete;
        Allocator& operator=(Allocator && other) = delete;
        
        size_t GetUsedMemory() const {return UsedMemory;}
        size_t GetSize() const {return Size;}
        size_t GetNumAllocations() const {return NumAllocations;}
        void* GetStart() const {return StartAddr;}
        
        virtual void* Allocate(size_t size, size_t alignment) = 0;
        virtual void* Reallocate(void *src, size_t size, size_t alignment) = 0;
        virtual void Free(void * ptr) = 0;
        
        protected:
        
        size_t Size; // size of the allocator
        void * StartAddr; // start address of the allocator
        size_t UsedMemory;
        size_t NumAllocations;
    };
    
    inline void * AlignAddress(void *addr, u8 alignment);
    inline u8 AlignLength(const void *addr, u8 alignment, u8 header_size = 0);
    
} // mm

#include "allocator.inl"

#endif //JENGINE_MM_ALLOCATOR_H
