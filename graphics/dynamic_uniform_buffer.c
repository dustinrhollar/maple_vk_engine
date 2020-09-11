
file_internal void mp_dynamic_uniform_buffer_init(mp_dynamic_uniform_buffer *Buffer, u64 TotalSize)
{
    u64 MinAlignment  = Core->VkCore.GetMinUniformMemoryOffsetAlignment();
    u32 SwapChainImageCount = Core->VkCore.GetSwapChainImageCount();
    
    //u64 BufferSize = (MinAlignment + TotalSize - 1) & ~(MinAlignment - 1);
    u64 BufferSize = memory_align(TotalSize, MinAlignment);
    
    Buffer->Alignment   = MinAlignment;
    Buffer->Size        = BufferSize;
    Buffer->Offset      = 0;
    Buffer->HandleCount = SwapChainImageCount;
    
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

file_internal void mp_dynamic_uniform_buffer_free(mp_dynamic_uniform_buffer *Buffer)
{
    for (u32 i = 0; i < Buffer->HandleCount; ++i)
    {
        Core->VkCore.DestroyVmaBuffer(Buffer->Handles[i].Handle, Buffer->Handles[i].Memory);
        Buffer->Handles[i].Size = 0;
    }
    
    memory_release(Core->Memory, Buffer->Handles);
    
    Buffer->HandleCount = 0;
    Buffer->Alignment   = 0;
    Buffer->Offset      = 0;
}

file_internal i32 mp_dynamic_uniform_buffer_alloc(mp_dynamic_uniform_buffer *Buffer, void *Data, u32 DataSize)
{
    i32 Result = -1; 
    
    u32 AlignedSize = memory_align(DataSize, Buffer->Alignment);
    
    if (Buffer->Offset + AlignedSize <= Buffer->Size)
    {
        u32 ImageIndex = Core->Renderer->CurrentImageIndex;
        buffer_parameters *Handle = Buffer->Handles + ImageIndex;
        
        char *Ptr = (char*)Handle->AllocationInfo.pMappedData + Buffer->Offset;
        memcpy(Ptr, Data, DataSize);
        
        Result = Buffer->Offset;
        Buffer->Offset += AlignedSize;
    }
    else
    {
        mprinte("Attempting to allocate space in a dynamic uniform buffer, but there is not enough space! Requested size: %d, Aligned Size: %d, Remaining Size: %d\n", DataSize, AlignedSize, Buffer->Size - Buffer->Offset);
    }
    
    return Result;
}

file_internal void mp_dynamic_uniform_buffer_reset(mp_dynamic_uniform_buffer *Buffer)
{
    // easy peasy
    Buffer->Offset = 0;
}
