#ifndef MAPLE_RESOURCES_RESOURCE_ALLOCATOR_H
#define MAPLE_RESOURCES_RESOURCE_ALLOCATOR_H

namespace mm
{
    struct resource_pool;
    
    struct resource_allocation
    {
        resource_pool       *Pool; // back pointer to the pool the allocation was from
        resource_allocation *Next; // Next free allocation
    };
    
    struct resource_pool
    {
        u32                  ElementStride;
        u32                  ElementCount;
        
        void                *Memory;
        
        resource_allocation *FreeList;
        
        resource_pool       *NextPool;
    };
    
    struct resource_allocator
    {
        resource_pool *Pools;
        u32            PoolsCount;
        
    };
    
    void  InitResourcePool(resource_pool *Pool, u32 Stride, u32 Count);
    void  FreeResourcePool(resource_pool *Pool);
    void* ResourcePoolRequestAllocation(resource_pool *Pool);
    void  ResourcePoolReleaseAllocation(resource_pool *Pool, void *Ptr);
    
    void InitResourceAllocator(resource_allocator *Allocator, u32 ElementStride, u32 InitialElementCount);
    void FreeResourceAllocator(resource_allocator *Allocator);
    
    void* ResourceAllocatorAlloc(resource_allocator *Allocator);
    void  ResourceAllocatorFree(resource_allocator *Allocator, void *Ptr);
}; //mm

#endif //MAPLE_RESOURCES_RESOURCE_ALLOCATOR_H
