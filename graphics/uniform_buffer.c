
void mp_uniform_buffer_init(mp_uniform_buffer *Buffer, u64 BufferSize)
{
    u32 SwapChainImageCount = Core->VkCore.GetSwapChainImageCount();
    
    Buffer->HandleCount = SwapChainImageCount;
    Buffer->Size        = BufferSize;
    
    VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    create_info.size  = BufferSize;
    create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    Buffer->Handles = (buffer_parameters*)memory_alloc(Core->Memory, sizeof(buffer_parameters) * SwapChainImageCount);
    for (u32 i = 0; i < SwapChainImageCount; ++i)
    {
        Core->VkCore.CreateVmaBuffer(create_info,
                                     alloc_info,
                                     Buffer->Handles[i].Handle,
                                     Buffer->Handles[i].Memory,
                                     Buffer->Handles[i].AllocationInfo);
        Buffer->Handles[i].Size = BufferSize;
    }
}

void mp_uniform_buffer_free(mp_uniform_buffer *Buffer)
{
    for (u32 i = 0; i < Buffer->HandleCount; ++i)
    {
        Core->VkCore.DestroyVmaBuffer(Buffer->Handles[i].Handle, Buffer->Handles[i].Memory);
        Buffer->Handles[i].Size = 0;
    }
    
    memory_release(Core->Memory, Buffer->Handles);
    Buffer->HandleCount = 0;
    Buffer->Size        = 0;
}

void mp_uniform_buffer_update(mp_uniform_buffer *Buffer, void *Data, u64 DataSize, u32 Offset)
{
    if (Offset + DataSize <= Buffer->Size)
    {
        u32 ImageIndex = Core->Renderer->CurrentImageIndex;
        
        buffer_parameters *Handle = Buffer->Handles + ImageIndex;
        char *Ptr = (char*)Handle->AllocationInfo.pMappedData + Offset;
        memcpy(Ptr + Offset, Data, DataSize);
    }
    else
    {
        mprinte("Attempting to allocate space in a uniform buffer, but there is not enough space! Requested size: %d, Remaining Size with offset: %d\n", DataSize, Buffer->Size - Offset);
    }
}
