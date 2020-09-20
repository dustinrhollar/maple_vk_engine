#ifndef GRAPHICS_UNIFORM_BUFFER_H
#define GRAPHICS_UNIFORM_BUFFER_H

typedef struct mp_uniform_buffer
{
    u64 Size;
    
    // One buffer per swapchain image. In order to avoid
    // overwriting memory currently being drawn, this is
    // necessary. "Offset" is the offset into the buffer
    // that is being written to in the current frame.
    buffer_parameters *Handles;
    u32                HandleCount;
    
} mp_uniform_buffer;

void mp_uniform_buffer_init(mp_uniform_buffer *Buffer, u64 Size);
void mp_uniform_buffer_free(mp_uniform_buffer *Buffer);
void mp_uniform_buffer_update(mp_uniform_buffer *Buffer, void *Data, u64 DataSize, u32 Offset);


#endif //GRAPHICS_UNIFORM_BUFFER_H
