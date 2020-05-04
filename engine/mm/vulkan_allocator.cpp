
namespace mm {
    
    VulkanProxyAllocator::VulkanProxyAllocator(Allocator &allocator)
        : ProxyAllocator(allocator)
    {
    }
    
    void* VulkanProxyAllocator::Allocation(size_t size,
                                           size_t alignment,
                                           VkSystemAllocationScope allocationScope)
    {
        return Allocate(size, alignment);
    }
    
    void* VKAPI_CALL VulkanProxyAllocator::Allocation(void* pUserData,
                                                      size_t size,
                                                      size_t alignment,
                                                      VkSystemAllocationScope allocationScope)
    {
        return static_cast<VulkanProxyAllocator*>(pUserData)->Allocation(size,
                                                                         alignment,
                                                                         allocationScope);
    }
    
    void* VulkanProxyAllocator::Reallocation(void* pOriginal,
                                             size_t size,
                                             size_t alignment,
                                             VkSystemAllocationScope allocationScope)
    {
        return Reallocate(pOriginal, size, alignment);
    }
    
    void* VKAPI_CALL VulkanProxyAllocator::Reallocation(void* pUserData,
                                                        void* pOriginal,
                                                        size_t size,
                                                        size_t alignment,
                                                        VkSystemAllocationScope allocationScope)
    {
        return static_cast<VulkanProxyAllocator*>(pUserData)->Reallocation(pOriginal,
                                                                           size,
                                                                           alignment,
                                                                           allocationScope);
    }
    
    void VulkanProxyAllocator::Free(void* pMemory)
    {
        InternalAllocator.Free(pMemory);
    }
    
    void VKAPI_CALL VulkanProxyAllocator::Free(void* pUserData,
                                               void* pMemory)
    {
        return static_cast<VulkanProxyAllocator*>(pUserData)->Free(pMemory);
    }
    
} // mm