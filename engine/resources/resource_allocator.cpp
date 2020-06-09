namespace mm
{
#define ResourceHeaderToMemory(h) (void*)((char*)(h) + sizeof(resource_pool*))
#define MemoryToResourceHeader(p) (resource_allocation*)((char*)(p) - sizeof(resource_pool*))
    
    void InitResourcePool(resource_pool *Pool, u32 Stride, u32 Count)
    {
        // do not allow for allocations less than a voind pointer
        assert(Stride >= sizeof(void*));
        
        // First, memory align the stride and make sure it is big enough to handle
        // the allocation header
        u32 Alignment = ALIGN(Stride + sizeof(resource_pool*));
        
        Pool->ElementStride = Alignment;
        Pool->ElementCount  = Count;
        
        Pool->Memory = palloc(Pool->ElementStride * Pool->ElementCount);
        
        // Setup the free list
        Pool->FreeList = (resource_allocation*)Pool->Memory;
        
        resource_allocation *Iter = Pool->FreeList;
        for (u32 HeaderIdx = 0; HeaderIdx < Count-1; ++HeaderIdx)
        {
            Iter->Next = (resource_allocation*)((char*)Iter +
                                                Pool->ElementStride); // set the next pointer to be the next allocation
            Iter = Iter->Next;
        }
        Iter->Next = nullptr;
        
        Pool->NextPool = nullptr;
    }
    
    void FreeResourcePool(resource_pool *Pool)
    {
        pfree(Pool->Memory);
        Pool->Memory        = nullptr;
        Pool->FreeList      = nullptr;
        Pool->NextPool      = nullptr;
        Pool->ElementStride = 0;
        Pool->ElementCount  = 0;
    }
    
    void* ResourcePoolRequestAllocation(resource_pool *Pool)
    {
        void *Result = nullptr;
        if (Pool->FreeList)
        {
            resource_allocation *Header = Pool->FreeList;
            
            Pool->FreeList = Pool->FreeList->Next;
            Header->Next   = nullptr;
            
            Header->Pool = Pool;
            Result = ResourceHeaderToMemory(Header);
        }
        
        return Result;
    }
    
    void ResourcePoolReleaseAllocation(resource_pool *Pool, void *Ptr)
    {
        resource_allocation *Header = MemoryToResourceHeader(Ptr);
        
        Header->Pool   = nullptr;
        Header->Next   = Pool->FreeList;
        Pool->FreeList = Header;
    }
    
    void InitResourceAllocator(resource_allocator *Allocator, u32 ElementStride, u32 InitialElementCount)
    {
        resource_pool *Pool = palloc<resource_pool>(1);
        InitResourcePool(Pool, ElementStride, InitialElementCount);
        
        Allocator->Pools = Pool;
        Allocator->PoolsCount = 1;
    }
    
    void FreeResourceAllocator(resource_allocator *Allocator)
    {
        resource_pool *Iter = Allocator->Pools;
        while (Iter)
        {
            resource_pool *ToFree = Iter;
            Iter = Iter->NextPool;
            
            FreeResourcePool(ToFree);
            pfree(ToFree);
        }
        
        Allocator->PoolsCount = 0;
    }
    
    void* ResourceAllocatorAlloc(resource_allocator *Allocator)
    {
        void *Result = nullptr;
        
        resource_pool *Iter = Allocator->Pools;
        while (Iter)
        {
            Result = ResourcePoolRequestAllocation(Iter);
            
            if (Result) break;
            Iter = Iter->NextPool;
        }
        
        if (!Result)
        {
            resource_pool *Pool = palloc<resource_pool>(1);
            InitResourcePool(Pool, Allocator->Pools->ElementStride,
                             Allocator->Pools->ElementCount*2);
            Pool->NextPool = Allocator->Pools;
            
            Allocator->Pools = Pool;
            Allocator->PoolsCount++;
            
            Result = ResourcePoolRequestAllocation(Pool);
            
            mprint("Adding a new resource pool!\n");
        }
        
        return Result;
    }
    
    void ResourceAllocatorFree(resource_allocator *Allocator, void *Ptr)
    {
        resource_allocation *Header = MemoryToResourceHeader(Ptr);
        ResourcePoolReleaseAllocation(Header->Pool, Ptr);
    }
    
#undef ResourceHeaderToMemory
#undef MemoryToResourceHeader
    
}; //mm