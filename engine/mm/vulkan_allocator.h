#ifndef SPLICER_ENGINE_VULKAN_ALLOCATOR_H
#define SPLICER_ENGINE_VULKAN_ALLOCATOR_H

namespace mm {
    
    class VulkanProxyAllocator : public ProxyAllocator {
        
        public:
        
        VulkanProxyAllocator(Allocator &proxy);
        ~VulkanProxyAllocator() = default;
        
        inline void GetVkAllocationCallbacks(VkAllocationCallbacks *result) const
        {
            result->pUserData             = (void*)this;
            result->pfnAllocation         = Allocation;
            result->pfnReallocation       = Reallocation;
            result->pfnFree               = Free;
            result->pfnInternalAllocation = nullptr;
            result->pfnInternalFree       = nullptr;
        }
        
        private:
        
        // Declare the allocator callbacks as static member functions.
        static void* VKAPI_CALL Allocation(void*                   pUserData,
                                           size_t                  size,
                                           size_t                  alignment,
                                           VkSystemAllocationScope allocationScope);
        
        static void* VKAPI_CALL Reallocation(void*                   pUserData,
                                             void*                   pOriginal,
                                             size_t                  size,
                                             size_t                  alignment,
                                             VkSystemAllocationScope allocationScope);
        
        static void VKAPI_CALL Free(void*                            pUserData,
                                    void*                            pMemory);
        
        // Now declare the nonstatic member functions that will actually perform
        // the allocations.
        void* Allocation(size_t                                      size,
                         size_t                                      alignment,
                         VkSystemAllocationScope                     allocationScope);
        
        void* Reallocation(void*                                     pOriginal,
                           size_t                                    size,
                           size_t                                    alignment,
                           VkSystemAllocationScope                   allocationScope);
        
        void Free(void*                                              pMemory);
        
    };
    
} // mm

#endif //SPLICER_ENGINE_VULKAN_ALLOCATOR_H
