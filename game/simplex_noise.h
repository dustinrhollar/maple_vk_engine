#ifndef GAME_SIMPLEX_NOISE_H
#define GAME_SIMPLEX_NOISE_H

typedef struct simplex_noisemap_sim_info
{
    i64 Seed;
    
    i32 NumOctaves;     // Control the number of octaves used
    r32 Persistence;    // Control the roughness of the output
    r32 Lacunarity;     // Control the width of "gaps" on the map 
    r32 Scale;          // Control the "zoom" for the noise
} simplex_noisemap_sim_info;

void simplex_reset_seed(i64 Seed);
void simplex_simulate_noisemap(heightmap *Heightmap, simplex_noisemap_sim_info *SimInfo);

#endif //GAME_SIMPLEX_NOISE_H
