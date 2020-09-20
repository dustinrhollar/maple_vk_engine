/* date = July 30th 2020 9:50 pm */

#ifndef ENGINE_GRAPHICS_VK_MAPLE_VK_H
#define ENGINE_GRAPHICS_VK_MAPLE_VK_H

#define VK_CHECK_RESULT(f, msg)                 \
{                                           \
VkResult res = (f);                     \
if ((res) != VK_SUCCESS)                \
{                                       \
PlatformFatalError(msg);            \
}                                       \
}

// Generate a push constant block.
// Usage Example:
//
// PushConstantsGen(push_constants)
// r32 Val0;
// vec3 Val1;
// PushConstantsEnd()
//
// The following example will produce the following code:
// union push_constants {
//     struct {
//         r32  Val0;
//         vec3 Val1;
//     };
//     struct { mat4 _Pad0, _Pad1; };
// };
//
// This generator will always produce a 128 byte PushConstant
// block if the user data is under 128 bytes. It is up to the
// user to make sure the generated push constant block does not
// exceed the 128 byte bounbdary from user-defined data.
#define PushConstantsGen(Name) union Name { struct {
#define PushConstantsEnd() }; struct { mat4 _Pad0, _Pad1; }; };

struct queue_family_indices
{
    std::optional<u32> graphicsFamily;
    std::optional<u32> presentFamily;
    
    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct queue_parameters
{
    VkQueue Handle;
    u32     FamilyIndex;
};

struct image_parameters
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

struct buffer_parameters
{
    VkBuffer          Handle;
    
    union 
    {
        VkDeviceMemory DeviceMemory;
        VmaAllocation  Memory;
    };
    
    VmaAllocationInfo AllocationInfo;
    size_t            Size;
};

struct descriptor_set_parameters
{
    //VkDescriptorPool      Pool;
    VkDescriptorSetLayout Layout;
    VkDescriptorSet       Handle;
};

struct swapchain_support_details
{
    VkSurfaceCapabilitiesKHR Capabilities;
    
    VkSurfaceFormatKHR      *Formats;
    u32                      FormatsCount;
    
    VkPresentModeKHR        *PresentModes;
    u32                      PresentModesCount;
};

struct depth_parameters
{
    image_parameters DepthImage;
};

struct framebuffer_parameters
{ // Have it contain Framebuffer and Depth Resources?
    VkFramebuffer Framebuffer;
};

struct swapchain_parameters
{
    VkSwapchainKHR  Handle; // set
    VkFormat        Format; // set
    VkExtent2D      Extent; // set
    
    image_parameters *Images; // Swapchain images
    u32             ImagesCount;
};

// Semaphore - between queues
// Fence     - whole queue operations to CPU
struct sync_object_parameters
{
    static constexpr int MAX_FRAMES = 2;
    
    VkSemaphore ImageAvailable[MAX_FRAMES];
    VkSemaphore RenderFinished[MAX_FRAMES];
    VkFence     InFlightFences[MAX_FRAMES];
    size_t      CurrentFrame = 0;
    
};

struct vulkan_core
{
    VkInstance             Instance;
    VkPhysicalDevice       PhysicalDevice;
    VkDevice               Device;
    queue_parameters       GraphicsQueue;
    queue_parameters       PresentQueue;
    VkSurfaceKHR           PresentationSurface;
    swapchain_parameters   SwapChain;
    
    // NOTE(Dustin): Might get moved to the frontend in order
    // to have per-thread sync objects
    sync_object_parameters SyncObjects;
    // Vulkan memory allocator
    VmaAllocator           VulkanAllocator;
    //jengine::mm::VulkanProxyAllocator *VkProxyAllocator;
    
    // MSAA
    VkSampleCountFlagBits  MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
    
    //~ Setup
    
    bool Init();
    void Shutdown();
    
    //~ Device Info
    
    image_parameters* GetSwapChainImages();
    u32 GetSwapChainImageCount();
    VkFormat GetSwapChainImageFormat();
    VkExtent2D GetSwapChainExtent();
    VkSampleCountFlagBits GetMaxUsableSampleCount();
    VkFormat FindDepthFormat();
    u64 GetMinUniformMemoryOffsetAlignment();
    
    void Idle();
    
    //~ Buffers
    
    void CreateVmaBuffer(VkBufferCreateInfo      buffer_create_info,
                         VmaAllocationCreateInfo vma_create_info,
                         VkBuffer                &buffer,
                         VmaAllocation           &allocation,
                         VmaAllocationInfo       &allocation_info);
    void CreateVmaBufferWithStaging(VkBufferCreateInfo      buffer_create_info,
                                    VmaAllocationCreateInfo vma_create_info,
                                    VkCommandPool           CommandPool,
                                    VkBuffer                &buffer,
                                    VmaAllocation           &allocation,
                                    void                    *data,
                                    VkDeviceSize            size);
    void DestroyVmaBuffer(VkBuffer buffer, VmaAllocation allocation);
    
    void CopyBuffer(VkCommandPool command_pool,
                    VkBuffer      src_buffer,
                    VkBuffer      dst_buffer,
                    VkDeviceSize  size);
    
    void VmaMap(void **mapped_memory, VmaAllocation allocation);
    void VmaUnmap(VmaAllocation allocation);
    void VmaFlushAllocation(VmaAllocation Allocation, VkDeviceSize Offset, VkDeviceSize Size);
    
    void Map(void           **mapped_memory,
             VkDeviceMemory   allocation,
             VkDeviceSize     offset,
             VkDeviceSize     size,
             VkMemoryMapFlags flags);
    void Unmap(VkDeviceMemory allocation);
    
    void BindVertexBuffers(VkCommandBuffer command_buffer,
                           u32             first_binding,
                           u32             binding_count,
                           VkBuffer        *buffers,
                           VkDeviceSize    *offsets);
    void BindIndexBuffer(VkCommandBuffer command_buffer,
                         VkBuffer        buffer,
                         VkDeviceSize    offset,
                         VkIndexType     index_type);
    
    //~ Image functions
    
    VkImageView CreateImageView(VkImageViewCreateInfo create_info);
    void DestroyImageView(VkImageView image_view);
    
    VkSampler CreateImageSampler(VkSamplerCreateInfo sampler_info);
    void DestroyImageSampler(VkSampler sampler);
    
    void CreateVmaImage(VkImageCreateInfo       image_create_info,
                        VmaAllocationCreateInfo vma_create_info,
                        VkImage                 &image,
                        VmaAllocation           &allocation,
                        VmaAllocationInfo       &allocation_info);
    void DestroyVmaImage(VkImage       image,
                         VmaAllocation allocation);
    
    void TransitionImageLayout(VkCommandPool command_pool,
                               VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout,
                               u32 mip_levels);
    
    void CopyBufferToImage(VkCommandPool command_pool,
                           VkBuffer buffer,
                           VkImage image,
                           u32 width, u32 height);
    
    //~ Command Buffer
    
    VkCommandPool CreateCommandPool(VkCommandPoolCreateFlags flags);
    void DestroyCommandPool(VkCommandPool command_pool);
    
    void CreateCommandBuffers(VkCommandPool        command_pool,
                              VkCommandBufferLevel level,
                              uint32_t             command_buffer_count,
                              VkCommandBuffer      *buffers);
    
    void DestroyCommandBuffers(VkCommandPool   command_pool,
                               u32             command_buffer_count,
                               VkCommandBuffer *buffers);
    
    void BeginCommandBuffer(VkCommandBuffer command_buffer);
    void EndCommandBuffer(VkCommandBuffer command_buffer);
    
    VkCommandBuffer BeginSingleTimeCommands(VkCommandPool command_pool);
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool command_pool);
    
    //~ Render Pass
    
    VkRenderPass CreateRenderPass(VkAttachmentDescription *attachments, 
                                  u32                      attachments_count,
                                  VkSubpassDescription    *subpasses, 
                                  u32                      subpass_count,
                                  VkSubpassDependency     *dependencies, 
                                  u32                      dependency_count);
    void DestroyRenderPass(VkRenderPass render_pass);
    void BeginRenderPass(VkCommandBuffer  command_buffer,
                         VkClearValue    *clear_values,
                         u32              clear_values_count,
                         VkFramebuffer    framebuffer,
                         VkRenderPass     render_pass);
    void EndRenderPass(VkCommandBuffer command_buffer);
    
    //~ Framebuffer 
    
    VkFramebuffer CreateFramebuffer(VkImageView *image_views, 
                                    u32          image_views_count,
                                    u32          image_index, 
                                    VkRenderPass render_pass);
    void DestroyFramebuffer(VkFramebuffer framebuffer);
    
    //~ Frame Setup
    
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
    
    //~ Pipeline
    
    VkShaderModule CreateShaderModule(const u32 *code, size_t size);
    void DestroyShaderModule(VkShaderModule module);
    
    void CreatePipelineCache(VkPipelineCache *PipelineCache);
    void DestroyPipelineCache(VkPipelineCache PipelineCache);
    
    VkPipeline CreatePipeline(VkGraphicsPipelineCreateInfo pipeline_info);
    VkPipeline CreatePipeline(VkGraphicsPipelineCreateInfo pipeline_info, VkPipelineCache PipelineCache);
    void DestroyPipeline(VkPipeline pipeline);
    
    VkPipelineLayout CreatePipelineLayout(VkPipelineLayoutCreateInfo layout_info);
    void DestroyPipelineLayout(VkPipelineLayout pipeline_layout);
    
    void BindPipeline(VkCommandBuffer command_buffer, VkPipeline pipeline);
    
    //~ Drawing
    
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
    
    //~ Create Descpriptor Sets
    
    VkDescriptorSetLayout CreateDescriptorSetLayout(VkDescriptorSetLayoutBinding *bindings, u32 bindings_count);
    void DestroyDescriptorSetLayout(VkDescriptorSetLayout layout);
    
    VkDescriptorPool CreateDescriptorPool(VkDescriptorPoolSize       *pool_sizes,
                                          u32                         pool_size_count,
                                          u32                         max_sets,
                                          VkDescriptorPoolCreateFlags flags);
    void DestroyDescriptorPool(VkDescriptorPool descriptor_pool);
    void ResetDescriptorPool(VkDescriptorPool      descriptor_pool);
    
    void CreateDescriptorSets(VkDescriptorSet            *descriptor_sets,
                              VkDescriptorSetAllocateInfo allocInfo);
    void DestroyDescriptorSets(VkDescriptorPool descriptor_pool,
                               VkDescriptorSet *descriptor_sets,
                               u32              descriptor_count);
    void UpdateDescriptorSets(VkWriteDescriptorSet *descriptor_set_writes,
                              u32                   write_count,
                              u32                   copy_count = 0,
                              VkCopyDescriptorSet*  pstop = NULL);
    void BindDescriptorSets(VkCommandBuffer  command_buffer,
                            VkPipelineLayout layout,
                            u32              first_set,
                            u32              descriptor_set_count,
                            VkDescriptorSet  *descriptor_sets,
                            u32              dynamic_offset_count,
                            u32              *dynamic_offsets);
    
    //~ Push Constants
    
    void PushConstants(VkCommandBuffer    CommandBuffer,
                       VkPipelineLayout   Layout,
                       VkShaderStageFlags StageFlags,
                       u32                Offset,
                       u32                Size,
                       const void*        pValues);
    
    //~ Helper Functions, should not be called outside of vulkan core
    
    private:
    
    bool CreateInstance();
    queue_family_indices FindQueueFamilies(VkPhysicalDevice physical_device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice physical_device);
    swapchain_support_details QuerySwapchainSupport(VkPhysicalDevice physical_device);
    bool IsDeviceSuitable(VkPhysicalDevice physical_device);
    void PickPhysicalDevice(VkInstance instance);
    void CreateLogicalDevice();
    void CreateSwapchain(swapchain_parameters &swapchain_params);
    void CreateSyncObjects(sync_object_parameters &sync_objects);
    
};

#endif //ENGINE_GRAPHICS_VK_MAPLE_VK_H
