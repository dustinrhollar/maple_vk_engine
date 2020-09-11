
typedef struct gauss_blur
{
    heightmap *Src;
    heightmap *Dst;
    
    u32  ChannelLength;
    i16 *SrcChannel;
    i16 *DstChannel;
    
    u32 Radius;
} gauss_blur;

// Boxes = an array of size = Count, Sigma = standard deviation, count = Number of Boxes
file_internal void gauss_boxes(i32 Boxes[], r32 Sigma, u32 Count);
file_internal void internal_gaussian_blur(gauss_blur *Blur);
file_internal void box_blurh(i16 *Src, i16 *Dst, u32 Length, u32 Width, u32 Height, u32 Radius);
file_internal void box_blurt(i16 *Src, i16 *Dst, u32 Length, u32 Width, u32 Height, u32 Radius);
file_internal void box_blur(i16 *Src, i16 *Dst, u32 Length, u32 Width, u32 Height, u32 Radius);


void gaussian_blur(heightmap *Src, heightmap *Dst, u32 Radius)
{
#if 1
    gauss_blur Blur = {0};
    Blur.Radius = Radius;
    Blur.Src = Src;
    Blur.Dst = Dst;
    Blur.ChannelLength = Src->Width * Src->Height;
    Blur.SrcChannel = (i16*)memory_alloc(PlatformApi->Memory, sizeof(i16) * Blur.ChannelLength);
    Blur.DstChannel = (i16*)memory_alloc(PlatformApi->Memory, sizeof(i16) * Blur.ChannelLength);
    
    // Copy the channel of interest into the src channel
    //memcpy(Blur.SrcChannel, Src->Ptr, sizeof(r32) * Blur.ChannelLength);
    for (u32 i = 0; i < Blur.ChannelLength; ++i) 
    {
        i16 n = Src->Ptr[i] * 255;
        //PlatformApi->mprint("%d\n", n);
        
        Blur.SrcChannel[i] = n; 
    }
    
    internal_gaussian_blur(&Blur);
    
    //memcpy(Dst->Ptr, Blur.DstChannel, sizeof(r32) * Blur.ChannelLength);
    for (u32 i = 0; i < Blur.ChannelLength; ++i) 
    {
        r32 Val = (r32)Blur.DstChannel[i] / 255.0f;
        //PlatformApi->mprint("%d %f\n", Blur.DstChannel[i], Val);
        
        Dst->Ptr[i] = Val; 
    }
    
    memory_release(PlatformApi->Memory, Blur.SrcChannel);
    memory_release(PlatformApi->Memory, Blur.DstChannel);
#else
    
    r32 *scl = Src->Ptr;
    r32 *tcl = Dst->Ptr;
    
    r32 rs = ceil(Radius * 2.57f);     // significant radius
    
    for(i32 i = 0; i < Src->Height; i++)
    {
        for(i32 j = 0; j < Src->Width; j++) 
        {
            r32 val = 0, wsum = 0;
            
            for(i32 iy = i - rs; iy < i + rs + 1; iy++)
            {
                for(i32 ix = j - rs; ix < j + rs + 1; ix++) 
                {
                    i32 x = min(Src->Width - 1, max(0, ix));
                    i32 y = min(Src->Height - 1, max(0, iy));
                    i32 dsq = (ix - j) * (ix - j) + (iy - i) * (iy - i);
                    
                    r32 wght = exp( -((r32)dsq) / (r32)(2.0f * Radius * Radius) ) / (JPI * 2.0f * Radius * Radius);
                    val += scl[y * Src->Width + x] * wght; 
                    wsum += wght;
                }
            }
            
            tcl[i * Src->Width + j] = round(val / wsum);            
        }
    }
    
#endif
}


file_internal void internal_gaussian_blur(gauss_blur *Blur)
{
    i32 Boxes[3];
    gauss_boxes(Boxes, Blur->Radius, 3);
    
    box_blur(Blur->SrcChannel, 
             Blur->DstChannel, 
             Blur->ChannelLength, 
             Blur->Src->Width, 
             Blur->Src->Height, 
             (Boxes[0] - 1) / 2);
    
    box_blur(Blur->DstChannel, 
             Blur->SrcChannel, 
             Blur->ChannelLength, 
             Blur->Src->Width, 
             Blur->Src->Height, 
             (Boxes[1] - 1) / 2);
    
    box_blur(Blur->SrcChannel, 
             Blur->DstChannel, 
             Blur->ChannelLength, 
             Blur->Src->Width, 
             Blur->Src->Height, 
             (Boxes[2] - 1) / 2);
}

file_internal void box_blur(i16 *Src, i16 *Dst, u32 Length, u32 Width, u32 Height, u32 Radius)
{
    //memcpy(Dst, Src, sizeof(r32) * Length);
    for (u32 i = 0; i < Length; ++i) Dst[i] = Src[i]; 
    
    box_blurh(Dst, Src, Length, Width, Height, Radius);
    box_blurt(Src, Dst, Length, Width, Height, Radius);
}

file_internal void gauss_boxes(i32 Boxes[], r32 Sigma, u32 Count)
{
    // calculate the idea averaging width
    r32 wIdeal = sqrt((12 * Sigma * Sigma / Count) + 1);
    i32 wl = (i32)wIdeal;
    if (wl % 2) wl--;
    
    i32 wu = wl + 2;
    
    r32 mIdeal = (12 * Sigma * Sigma - Count * wl * wl - 4 * Count * wl - 3 * Count) / (-4 * wl - 4);
    i32 m = (i32)round(mIdeal);
    
    for (u32 i = 0; i < Count; ++i) Boxes[i] = (i < m) ? wl : wu;
}

file_internal void box_blurh(i16 *Src, i16 *Dst, u32 Length, u32 Width, u32 Height, u32 Radius)
{
    r32 iarr = (r32)1 / (r32)(Radius + Radius + 1);
    for(u32 i = 0; i < Height; i++) {
        u32 ti = i * Width;
        u32 li = ti;
        u32 ri = ti + Radius;
        
        i16 fv  = Src[ti];
        i16 lv  = Src[ti + Width - 1];
        i16 val = (Radius + 1) * fv;
        
        for(u32 j=0; j<Radius; j++) 
            val += Src[ti + j];
        
        for(u32 j = 0; j <= Radius; j++) 
        { 
            val += Src[ri++] - fv;   
            Dst[ti++] = round(val * iarr); 
        }
        
        for(u32 j = Radius + 1; j < Width - Radius; j++) 
        { 
            val += Src[ri++] - Dst[li++];   
            Dst[ti++] = round(val * iarr); 
        }
        
        for(u32 j = Width - Radius; j < Width; j++) 
        { 
            val += lv - Src[li++];   
            Dst[ti++] = round(val*iarr); 
        }
    }
}

file_internal void box_blurt(i16 *Src, i16 *Dst, u32 Length, u32 Width, u32 Height, u32 Radius)
{
    r32 iarr = (r32)1 / (r32)(Radius + Radius + 1);
    
    for(u32 i=0; i<Width; i++) 
    {
        u32 ti = i;
        u32 li = ti;
        u32 ri = ti + Radius * Width;
        
        i16 fv = Src[ti];
        i16 lv = Src[ti + Width * (Height- 1)];
        i16 val = (Radius + 1) * fv;
        
        for(u32 j = 0; j < Radius; j++) 
            val += Src[ti + j * Width];
        
        for(u32 j = 0; j <= Radius; j++) 
        { 
            val += Src[ri] - fv;  
            Dst[ti] = round(val * iarr);  
            ri += Width; 
            ti += Width; 
        }
        
        for(u32 j = Radius + 1; j < Height - Radius; j++) 
        { 
            val += Src[ri] - Src[li];  
            Dst[ti] = round(val * iarr);  
            li += Width; 
            ri += Width; 
            ti += Width; 
        }
        
        for(u32 j = Height - Radius; j < Height; j++) 
        { 
            val += lv - Src[li];  
            Dst[ti] = round(val * iarr);  
            li += Width; 
            ti += Width; 
        }
    }
}
