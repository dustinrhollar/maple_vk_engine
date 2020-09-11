#ifndef GAME_HEIGHTMAP_H
#define GAME_HEIGHTMAP_H

typedef struct heightmap
{
    upload_buffer Handle;
    u32 Width, Height;
    r32 *Ptr; // leave the heightmap mapped to make it easier to access data
} heightmap;

void heightmap_init(heightmap *Heightmap, u32 Width, u32 Height);
void heightmap_free(heightmap *Heightmap);
void heightmap_resize(heightmap *Heightmap, u32 Width, u32 Height);
r32 heightmap_sample(heightmap *Heightmap, vec2 Uv);

#endif //GAME_HEIGHTMAP_H
