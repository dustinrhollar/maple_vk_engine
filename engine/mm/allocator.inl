#include "allocator.h"

namespace mm {

inline void * AlignAddress(void *addr, uint8_t alignment)
{
    return (void*)((reinterpret_cast<uptr>(addr) + static_cast<uptr>(alignment-1)) & static_cast<uptr>(~(alignment-1)));
}

inline uint8_t AlignLength(const void *addr, uint8_t alignment, uint8_t header_size)
{
    uint8_t adj = alignment - (reinterpret_cast<uptr>(addr) & static_cast<uptr>(alignment-1));
    adj = (adj == alignment) ? 0 : adj;
    
    uint8_t required = header_size;
    
    if (adj < required)
    {
        required -= adj;
        
        adj += alignment * (required / alignment);
        
        if (required % alignment > 0) adj += alignment;
    }
    
    return adj;
}

} // mm
