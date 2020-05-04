#ifndef JENGINE_MM_MM_H
#define JENGINE_MM_MM_H

/*

@brief: just kidding. This isn't brief.

Memory Manager is an interface to heap allocation
for a program. There are two types of memory provide:
1. Permanant Storage
2. Transient Storage

Permanant Storage is the default memory storage for the
managaer unless otherwise specified. This type of storage
is for allocations that are not expected to be freed/allocated
on regular basis. Permanant Storage managed through a
Free List Allocator.

Transient Storage is temporary storage that is expected
to be freed on a regular basis. For example, a good usage
would be to use per-Frame allocations for transient storage.
Internally, a Linear Allocator is used to manage Transient
Memory, which means that memory is freed (reset) for the
entire Allocator rather than on a per-allocation basis.
Allocations from Transient Storage should not call Free(),
but call Reset() for the entire allocator. If it is expected
that a user need to free on a per-allocation basis, it is
recommended that a sub-allocator is requested from Transient
Memory and memory is managed through the sub-allocator instead.

To initialize memory manager, a size of allocation and the size
of transient memory is requested. Note that "size of allocation"
refers to the entire memory block that will be allocated. The size
of permanant storage will be:
          (size of allocation) - (size of transient memory)

Three internal pointers are managed within the Memory Manager.
1. void *GlobalMemory: pointer to the start of the global allocated memory
2. FreeListAllocator *PermanantStorage: pointer to the FreeListAllocator object
3. LinearAllocator *TransientStorage: pointer to the LinearAllocator object

Since pointers cannot be created on the stack, space for them must
be allocated at runtime. In order to allocate space for these two objects,
additional memory is appended to the front of the GlobalMemory. If the Memory
Manager is intialized with M bytes, then a total allocation of size:
            M + sizeof(FreeListAllocator) + sizeof(LinearAllocator)
is made. Therefore, the GlobalMemory block will have the following memory layout:

     //                                                  | ---------- Requested Size --------- |
-------------------------------------------------------------------------------------------
| FreeListAllocator Object | LinearAllocator object | Transient Memory | Permanant Memory |
-------------------------------------------------------------------------------------------

NOTE: Diagram not to scale :p

When shutting down the Memory Manager, it is expected that a user has
managed memory appropriately. While this is not a concern for
TransientMemory (allocator is simply reset), PermanantMemory will
trigger an assertion error if there are allocations that have not
been freed. This is done in order to assist users in detecting
memory leaks at program shutdown. If this not behavior a user
desires, then remove the lines in "ShutdownMemoryManager" that call
the destructors for the PermanantMemory and Transient memory in
"mm.cpp".

*/

namespace mm {
    
    /*

    Permanant Storage is for allocations that are expected
    to have infrequent frees and allocations. Use this memory
    for allocations that last for most of the program's lifetime.

    Transient Storage is for allocations that are expected
    to have frequent allocations and freed all at once. Use this for memory
    that will be reset at the end of a frame.

    */
    
    enum MemoryRequest
    {
        MEMORY_REQUEST_TRANSIENT_STORAGE   = 1<<0,
        MEMORY_REQUEST_PERMANANT_STORAGE   = 1<<1,
        MEMORY_REQUEST_LINEAR_ALLOCATOR    = 1<<2,
        MEMORY_REQUEST_STACK_ALLOCATOR     = 1<<3,
        MEMORY_REQUEST_FREE_LIST_ALLOCATOR = 1<<4,
        MEMORY_REQUEST_POOL_ALLOCATOR      = 1<<5,
    };
    
    /*

    Initializes the memory manager and the Permanant/Transient storage allocators.
    @param memory_to_reserve: total memory that is allocated from the operating system
    @param memory_for_transient_storage: memory that is allocated from total storage for
    transient storage. The remaining memory is used for Permanant Storage.


    */
    void InitializeMemoryManager(size_t memory_to_reserve, size_t memory_for_transient_storage);
    void ShutdownMemoryManager();
    
    // Resets transient memory
    void ResetTransientMemory();
    
    /*

    Allocates raw memory from main allocators: Permanent/Transient
    By default, requests go to Permanant storage
    Only Permanant and Transient storage are supported by jalloc.
    Nullptr is returned if the allocation fails or an invalid memory
    request is made.

    Allocations are made with 8-byte alignment.

    */
    void* jalloc(size_t size, MemoryRequest memory_request = MEMORY_REQUEST_PERMANANT_STORAGE);
    template<class T> T* jalloc(size_t num_elements = 1, MemoryRequest memory_request = MEMORY_REQUEST_PERMANANT_STORAGE)
    {
        return (T*)jalloc(num_elements * sizeof(T), memory_request);
    }
    
    /*

    Frees raw memory from the main allocators: Permanent/Transient
    By default, requests go to Permanant storage.
    Only Permanant and Transient storage are supported by jfree.

    Make sure the memory_request is of the same type as the allocation.
    Behavior is undefined for incorrect memory_requests.

    */
    void jfree(void *ptr, MemoryRequest memory_request = MEMORY_REQUEST_PERMANANT_STORAGE);
    template<class T> void jfree(T *ptr, MemoryRequest memory_request = MEMORY_REQUEST_PERMANANT_STORAGE)
    {
        jfree((void*)ptr, memory_request);
    }
    
    /*

    Allocates memory from a particular allocator.
    Allocate a single object of type T and copy the parameter "t" to it.

    By default, memory is allocated from permanant sotrage.

    */
    
    template <class T> T* jalloc(Allocator & allocator, T & t)
    {
        return new (allocator.Allocate(sizeof(T), __alignof(T))) T(t);
    }
    
    /*

    Allocations memory from an allocator by the number of elements
    provided and their type. Allocations are algined based on the
    requested memory type.

    Memory is zero'd upon allocation (?).

    By default, memory is allocated from permanant sotrage.

    */
    template <class T> T* jalloc(Allocator & allocator, size_t num_elements = 1,
                                 MemoryRequest memory_request = MEMORY_REQUEST_PERMANANT_STORAGE)
    {
        size_t alignment = __alignof(T);
        size_t size = sizeof(T) * num_elements;
        
        T* addr = (T*)allocator.Allocate(size, alignment);
        
        return addr;
    }
    
    /*

    Frees an allocation from an allocator. The destructor for T is not
    called by this function or the allocator. Since array-type allocations
    are not exlicitly handled by the MemoryManager, the number elements
    for array-type allocations are not known. Therefore it is up to the user
    to handle clearing state that might be contained within T.

    */
    template <class T> void jfree(Allocator & allocator, T & t,
                                  MemoryRequest memory_request = MEMORY_REQUEST_PERMANANT_STORAGE)
    {
        allocator.Free(t);
    }
    
    void *jrealloc(void *src, size_t size, MemoryRequest memory_request = MEMORY_REQUEST_PERMANANT_STORAGE);
    
    Allocator *GetPermanantStorage();
    
} // mm

void *talloc(size_t size);
template<class T> T* talloc(size_t num_elements = 1)
{
    return mm::jalloc<T>(num_elements, mm::MEMORY_REQUEST_TRANSIENT_STORAGE);
}

void *palloc(size_t size);
template<class T> T* palloc(size_t num_elements = 1)
{
    return (T*)palloc(sizeof(T) * num_elements);
}

template<class T> void pfree(T *ptr)
{
    mm::jfree(ptr, mm::MEMORY_REQUEST_PERMANANT_STORAGE);
}


void *prealloc(void *src, size_t size);
template<class T> T* prealloc(T *src, size_t num_elements = 1)
{
    return (T*)prealloc((void*)src, sizeof(T) * num_elements);
}

void *trealloc(void *src, size_t size);
template<class T> T *trealloc(void *src, size_t num_elements)
{
    return (T*)trealloc(src, sizeof(T) * num_elements);
}

#endif // JENGINE_MM_MM_H
