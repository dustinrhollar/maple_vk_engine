
namespace mm {
    
    StackAllocator::StackAllocator(size_t size, void *start)
        : Allocator(size, start)
        , PrevPos(nullptr)
        , CurPos(start)
    {
    }
    
    StackAllocator::~StackAllocator()
    {
        CurPos = nullptr;
        PrevPos = nullptr;
    }
    
    
    void * StackAllocator::Allocate(size_t size, size_t alignment)
    {
        u8 align_length = AlignLength(CurPos, alignment, sizeof(Header));
        void *align_addr = (void*)((char*)CurPos + align_length);
        
        Header* header = Mem2Header(align_addr);
        header->Alignment = align_length;
        header->PrevAddr = PrevPos;
        
        PrevPos = align_addr;
        CurPos = (char*)align_addr + size;
        ++NumAllocations;
        UsedMemory += align_length + size;
        
        return align_addr;
    }
    
    void* StackAllocator::Reallocate(void *src, size_t size, size_t alignment) {
        void *result = Allocate(size, alignment);
        memcpy(result, src, size);
        Free(src);
        
        return result;
    }
    
    
    void StackAllocator::Free(void * ptr)
    {
        /*
        -------------
        | A | B | C |
        -------------
    
        Call Free(A)
            PrevPos = C
            CurPos = next Allocation
    
            First, call Free(C)
                Prev = B
                CurPos = where C was
            
            Then, call Free(B)
                PrevPos = A
                CurPos = where B was
    
            Finally call Free(A)
                PrevPos = nullptr
                CurPos = where A was
        */
        
        // Remove allocations from the stack until the requested address is found
        void* removed = nullptr;
        while (removed != ptr)
        {
            void *addr = PrevPos;
            Header *addr_header = Mem2Header(addr);
            
            UsedMemory -= ((char*)CurPos - (char*)addr) + addr_header->Alignment;
            --NumAllocations;
            
            CurPos = (char*)addr - addr_header->Alignment;
            PrevPos = Mem2Header(PrevPos)->PrevAddr;
            
            removed = addr;
        }
    }
    
} // mm