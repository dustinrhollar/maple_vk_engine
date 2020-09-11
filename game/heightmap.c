
void heightmap_init(heightmap *Heightmap, u32 Width, u32 Height)
{
    Heightmap->Width = Width;
    Heightmap->Height = Height;
    
    u64 Size = Heightmap->Width * Heightmap->Height * sizeof(r32);
    Graphics.create_upload_buffer(&Heightmap->Handle, UploadBuffer_Texture, Size);
    Graphics.map_upload_buffer(Heightmap->Handle, 0, (void**) &Heightmap->Ptr);
}

void heightmap_free(heightmap *Heightmap)
{
    if (Heightmap->Ptr)    Graphics.unmap_upload_buffer(Heightmap->Handle, (void**) &Heightmap->Ptr);
    if (Heightmap->Handle) Graphics.free_upload_buffer(&Heightmap->Handle);
}

void heightmap_resize(heightmap *Heightmap, u32 Width, u32 Height)
{
    Heightmap->Width  = Width;
    Heightmap->Height = Height;
    
    u64 Size = Heightmap->Width * Heightmap->Height * sizeof(r32);
    // if the buffer size has not increased, then the buffer is not actually resized.
    Graphics.resize_upload_buffer(Heightmap->Handle, Size);
    Graphics.map_upload_buffer(Heightmap->Handle, 0, (void**) &Heightmap->Ptr);
}

/*

h2         h3
+----------+   0
|          |
|          |
|     X    |
|          |  
+----------+   1
          h0         h1

0          1

*/

// NOTE(Dustin): If one of the upper edges of the heightmap
// is being sampled, the lerp wraps to the lower edge in
// in order to get a good sample. I need to do more
// research on better ways to do this.
r32 heightmap_sample(heightmap *Heightmap, vec2 Uv)
{
    r32 LerpX = Heightmap->Width * Uv.x;     // 5.33 -> t = 0.33
    r32 LerpY = Heightmap->Height * Uv.y;   // 10.43 -> s = 0.43
    
    i32 x = (i32)LerpX;
    i32 y = (i32)LerpY;
    
    r32 HeightUvX = LerpX - x;
    r32 HeightUvY = LerpY - y;
    
    i32 h0x = x + 0;
    i32 h0y = y + 0;
    
    i32 h1x = (x + 1) % Heightmap->Width;
    i32 h1y = y + 0;
    
    i32 h2x = x + 0;
    i32 h2y = (y + 1) % Heightmap->Height;
    
    i32 h3x = h1x;
    i32 h3y = h2y;
    
    r32 h0 = Heightmap->Ptr[h0y * Heightmap->Width + h0x];
    r32 h1 = Heightmap->Ptr[h1y * Heightmap->Width + h1x];
    r32 h2 = Heightmap->Ptr[h2y * Heightmap->Width + h2x];
    r32 h3 = Heightmap->Ptr[h3y * Heightmap->Width + h3x];
    
    r32 r1 = lerp(h0, h1, HeightUvX);
    r32 r2 = lerp(h2, h3, HeightUvX);
    
    return lerp(r1, r2, HeightUvY);
}
