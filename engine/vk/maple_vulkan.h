#ifndef SPLICER_ENGINE_VULKAN_H
#define SPLICER_ENGINE_VULKAN_H

#define VK_CHECK_RESULT(f, msg)                 \
{                                           \
    VkResult res = (f);                     \
    if ((res) != VK_SUCCESS)                \
    {                                       \
        throw std::runtime_error(msg);      \
    }                                       \
}

struct VulkanCore;
// defined in: splicer_vulkan.cpp

struct QueueFamilyIndices
{
    std::optional<u32> graphicsFamily;
    std::optional<u32> presentFamily;
    
    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct QueueParameters
{
    VkQueue Handle;
    u32     FamilyIndex;
};

struct ImageParameters
{
    VkImage        Handle;
    VkImageView    View;
    VkSampler      Sampler;
    
    union {
        VkDeviceMemory DeviceMemory;
        VmaAllocation  Memory;
    };
    
    VmaAllocationInfo AllocationInfo;
    
    //VkDeviceMemory Memory;
};

struct BufferParameters
{
    VkBuffer          Handle;
    
    union {
        VkDeviceMemory DeviceMemory;
        VmaAllocation  Memory;
    };
    
    VmaAllocationInfo AllocationInfo;
    size_t            Size;
};

struct DescriptorSetParameters
{
    VkDescriptorPool      Pool;
    VkDescriptorSetLayout Layout;
    VkDescriptorSet       Handle;
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR Capabilities;
    
    VkSurfaceFormatKHR *Formats;
    u32                 FormatsCount;
    
    VkPresentModeKHR   *PresentModes;
    u32                 PresentModesCount;
};

struct DepthParameters
{
    ImageParameters DepthImage;
};

struct FramebufferParameters
{ // Have it contain Framebuffer and Depth Resources?
    VkFramebuffer Framebuffer;
};

struct SwapChainParameters
{
    VkSwapchainKHR  Handle; // set
    VkFormat        Format; // set
    VkExtent2D      Extent; // set
    
    ImageParameters *Images; // Swapchain images
    u32             ImagesCount;
};

// Semaphore - between queues
// Fence     - whole queue operations to CPU
struct SyncObjectParameters
{
    static constexpr int MAX_FRAMES = 2;
    
    VkSemaphore ImageAvailable[MAX_FRAMES];
    VkSemaphore RenderFinished[MAX_FRAMES];
    VkFence     InFlightFences[MAX_FRAMES];
    size_t      CurrentFrame = 0;
    
};

struct VulkanCore
{
    VkInstance          Instance;
    VkPhysicalDevice    PhysicalDevice;
    VkDevice            Device;
    QueueParameters     GraphicsQueue;
    QueueParameters     PresentQueue;
    VkSurfaceKHR        PresentationSurface;
    SwapChainParameters SwapChain;
    
    // NOTE(Dustin): Might get moved to the frontend in order
    // to have per-thread sync objects
    SyncObjectParameters SyncObjects;
    // Vulkan memory allocator
    VmaAllocator         VulkanAllocator;
    //jengine::mm::VulkanProxyAllocator *VkProxyAllocator;
    
    // MSAA
    VkSampleCountFlagBits MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
};

// defined in splicer_vulkan.cpp

namespace vk {
    
    // defined in splicer_vulkan.cpp
    extern VulkanCore GlobalVulkanState;
    
    bool InitializeVulkan();
    void ShutdownVulkan();
    
    void Idle();
    
    ImageParameters* GetSwapChainImages();
    VkExtent2D GetSwapChainExtent();
    u32 GetSwapChainImageCount();
    VkFormat GetSwapChainImageFormat();
    VkFormat FindDepthFormat();
    VkSampleCountFlagBits GetMaxMsaaSamples();
    
    VkCommandBuffer BeginSingleTimeCommands(VkCommandPool command_pool);
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool command_pool);
    
    VkImageView CreateImageView(VkImageViewCreateInfo create_Info);
    void DestroyImageView(VkImageView image_view);
    
    void CreateVmaImage(VkImageCreateInfo       image_create_info,
                        VmaAllocationCreateInfo vma_create_info,
                        VkImage                 &image,
                        VmaAllocation           &allocation,
                        VmaAllocationInfo       &allocation_info);
    void DestroyVmaImage(VkImage       image,
                         VmaAllocation allocation);
    
    void CreateImage(VkImageCreateInfo       image_create_info,
                     VmaAllocationCreateInfo vma_create_info,
                     VkImage                 &image,
                     VkDeviceMemory          &allocation);
    void DestroyImage(VkImage        image,
                      VkDeviceMemory allocation);
    
    VkSampler CreateImageSampler(VkSamplerCreateInfo sampler_info);
    void DestroyImageSampler(VkSampler sampler);
    
    void TransitionImageLayout(VkCommandPool command_pool,
                               VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout,
                               u32 mip_levels);
    
    VkRenderPass CreateRenderPass(VkAttachmentDescription *attachments, u32 attachments_count,
                                  VkSubpassDescription *subpasses, u32 subpass_count,
                                  VkSubpassDependency *dependencies, u32 dependency_count);
    void DestroyRenderPass(VkRenderPass render_pass);
    
    VkFormat FindDepthFormat();
    
    VkCommandPool CreateCommandPool(VkCommandPoolCreateFlags flags);
    void DestroyCommandPool(VkCommandPool command_pool);
    
    // the SwapChain Image Views are appended to the passes image_views list
    // @param image_index: the index of the swapchain image to attach the framebuffer to
    VkFramebuffer CreateFramebuffer(VkImageView *image_views, u32 images_views_count,
                                    u32 image_index, VkRenderPass render_pass);
    void DestroyFramebuffer(VkFramebuffer framebuffer);
    
    void DestroyMemory(VkDeviceMemory memory);
    
    void CreateCommandBuffers(VkCommandPool        command_pool,
                              VkCommandBufferLevel level,
                              uint32_t             command_buffer_count,
                              VkCommandBuffer      *buffers);
    
    void DestroyCommandBuffers(VkCommandPool   command_pool,
                               u32             command_buffer_count,
                               VkCommandBuffer *buffers);
    
    // @return false if the swapchain was out of date and a resize occured. true otherwise
    VkResult BeginFrame(u32          &next_image_idx);
    // @param current_frame_index, represents the current frame. Needed in order to retrieve
    //                             the current swapchain and semaphore/fence values. It should
    //                             be noted that this value is modified to represent the next frame
    //                             upon completion of this function.
    // @return true if the swapchain was out of date and a resize occured. False otherwise
    void EndFrame(u32             current_image_index,
                  VkCommandBuffer *command_buffers, /* used for the wait stage - buffer we wait for to complete */
                  u32             command_buffer_count);
    
    void SetViewport(VkCommandBuffer command_buffer,
                     u32             first_viewport,
                     u32             viewport_count,
                     VkViewport      *viewport);
    
    void SetScissor(VkCommandBuffer command_buffer,
                    u32             first_scissor,
                    u32             scissor_count,
                    VkRect2D        *scissor);
    
    void BeginCommandBuffer(VkCommandBuffer command_buffer);
    void EndCommandBuffer(VkCommandBuffer command_buffer);
    
    void BeginRenderPass(VkCommandBuffer command_buffer,
                         VkClearValue    *clear_values,
                         u32             clear_values_count,
                         VkFramebuffer   framebuffer,
                         VkRenderPass    render_pass);
    void EndRenderPass(VkCommandBuffer command_buffer);
    
    void Resize();
    
    void CreateVmaBuffer(VkBufferCreateInfo      buffer_create_info,
                         VmaAllocationCreateInfo vma_create_info,
                         VkBuffer                &buffer,
                         VmaAllocation           &allocation,
                         VmaAllocationInfo       &allocation_info);
    void DestroyVmaBuffer(VkBuffer buffer, VmaAllocation allocation);
    
    void CreateBuffer(VkBufferCreateInfo      buffer_create_info,
                      VkMemoryAllocateInfo    vma_create_info,
                      VkBuffer                &buffer,
                      VkDeviceMemory          &allocation);
    void DestroyBuffer(VkBuffer buffer, VkDeviceMemory allocation);
    
    void DestroyDeviceMemory(VkDeviceMemory device_memory);
    
    // Uploads the buffer to the GPU via a staging buffer
    void  CreateVmaBufferWithStaging(VkCommandPool           command_pool,
                                     VkBufferCreateInfo      buffer_create_info,
                                     VmaAllocationCreateInfo vma_create_info,
                                     VkBuffer                &buffer,
                                     VmaAllocation           &allocation,
                                     void                    *data,
                                     VkDeviceSize            size);
    void  CreateBufferWithStaging(VkCommandPool           command_pool,
                                  VkBufferCreateInfo      buffer_create_info,
                                  VkMemoryAllocateInfo    memory_create_info,
                                  VkBuffer                &buffer,
                                  VkDeviceMemory          &allocation,
                                  void                    *data,
                                  VkDeviceSize            size);
    
    void CopyBuffer(VkCommandPool command_pool,
                    VkBuffer src_buffer,
                    VkBuffer dst_buffer,
                    VkDeviceSize size);
    
    void VmaMap(void **mapped_memory, VmaAllocation allocation);
    void VmaUnmap(VmaAllocation allocation);
    
    void Map(void             **mapped_memory,
             VkDeviceMemory   allocation,
             VkDeviceSize     offset,
             VkDeviceSize     size,
             VkMemoryMapFlags flags);
    void Unmap(VkDeviceMemory allocation);
    
    VkShaderModule CreateShaderModule(const char *code, size_t size);
    void DestroyShaderModule(VkShaderModule module);
    
    VkPipeline CreatePipeline(VkGraphicsPipelineCreateInfo pipeline_info);
    void DestroyPipeline(VkPipeline pipeline);
    
    VkPipelineLayout CreatePipelineLayout(VkPipelineLayoutCreateInfo layout_info);
    void DestroyPipelineLayout(VkPipelineLayout pipeline_layout);
    
    void BindPipeline(VkCommandBuffer command_buffer, VkPipeline pipeline);
    
    VkDescriptorSetLayout CreateDescriptorSetLayout(VkDescriptorSetLayoutBinding *bindings, u32 bindings_count);
    void DestroyDescriptorSetLayout(VkDescriptorSetLayout layout);
    
    VkDescriptorPool CreateDescriptorPool(VkDescriptorPoolSize *pool_sizes,
                                          u32 pool_size_count,
                                          u32 max_sets,
                                          VkDescriptorPoolCreateFlags flags = 0);
    void DestroyDescriptorPool(VkDescriptorPool descriptor_pool);
    
    void ResetDescriptorPool(VkDescriptorPool descriptor_pool);
    
    VkResult CreateDescriptorSets(VkDescriptorSet             *descriptor_sets,
                                  VkDescriptorSetAllocateInfo allocInfo);
    void DestroyDescriptorSets(VkDescriptorPool descriptor_pool,
                               VkDescriptorSet  *descriptor_sets,
                               u32              descriptor_count);
    
    void UpdateDescriptorSets(VkWriteDescriptorSet *descriptor_set_writes,
                              u32 write_count,
                              u32 copy_count = 0,
                              VkCopyDescriptorSet* pstop = nullptr);
    
    size_t GetMinUniformMemoryOffsetAlignment();
    
    void Draw(VkCommandBuffer command_buffer,
              u32             vertex_count,
              u32             instance_count,
              u32             first_vertex,
              u32             first_instance);
    void DrawIndexed(VkCommandBuffer command_buffer,
                     u32             index_count,
                     u32             instance_count,
                     u32             first_index,
                     u32             vertex_offset,
                     u32             first_instance);
    
    void BindVertexBuffers(VkCommandBuffer command_buffer,
                           u32             first_binding,
                           u32             binding_count,
                           VkBuffer        *buffers,
                           VkDeviceSize    *offsets);
    void BindIndexBuffer(VkCommandBuffer command_buffer,
                         VkBuffer        buffer,
                         VkDeviceSize    offset,
                         VkIndexType     index_type);
    
    void BindDescriptorSets(VkCommandBuffer  command_buffer,
                            VkPipelineLayout layout,
                            u32              first_set,
                            u32              descriptor_set_count,
                            VkDescriptorSet  *descriptor_sets,
                            u32              dynamic_offset_count,
                            u32              *dynamic_offsets);
    
    void CopyBufferToImage(VkCommandPool command_pool,
                           VkBuffer buffer,
                           VkImage image,
                           u32 width, u32 height);
    
    void CopyBufferToImage(VkCommandPool command_pool,
                           VkBuffer buffer,
                           VkImage image,
                           u32 width, u32 height);
    
    void GenerateMipmaps(VkCommandPool command_pool,
                         VkImage image, VkFormat image_format,
                         i32 width, i32 height, u32 mip_levels);
    
}

#endif //SPLICER_ENGINE_VULKAN_H
