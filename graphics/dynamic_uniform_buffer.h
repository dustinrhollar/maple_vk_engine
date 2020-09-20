/* date = August 22nd 2020 3:48 pm */

#ifndef GRAPHICS_DYNAMIC_UNIFORM_BUFFER_H
#define GRAPHICS_DYNAMIC_UNIFORM_BUFFER_H

// A simple linear allocator 
typedef struct mp_dynamic_uniform_buffer
{
    u32                Alignment;
    u64                Size;
    u32                Offset;
    
    // One buffer per swapchain image. In order to avoid
    // overwriting memory currently being drawn, this is
    // necessary. "Offset" is the offset into the buffer
    // that is being written to in the current frame.
    buffer_parameters *Handles;
    u32                HandleCount;
    
} mp_dynamic_uniform_buffer;

file_internal void mp_dynamic_uniform_buffer_init(mp_dynamic_uniform_buffer *Buffer, u64 TotalSize);
file_internal void mp_dynamic_uniform_buffer_free(mp_dynamic_uniform_buffer *Buffer);
file_internal i32 mp_dynamic_uniform_buffer_alloc(mp_dynamic_uniform_buffer *Buffer, void *Data, u32 DataSize);
file_internal void mp_dynamic_uniform_buffer_reset(mp_dynamic_uniform_buffer *Buffer);

#endif //GRAPHICS_DYNAMIC_UNIFORM_BUFFER_H
