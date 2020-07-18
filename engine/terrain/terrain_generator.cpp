//
// Created by Dustin Hollar on 10/15/18.
// Documentation located in the header file
//

//#include <OpenSimplexNoise.h>
//OpenSimplexNoise openSimplex;

// --------------------------------------------------------------------------------------------- //
// Find the Biome that exists at the passed height value.
// --------------------------------------------------------------------------------------------- //
Biome::biome Biome::GetBiome(r32 Height)
{
    if      ( Height < 0.1f ) return Biome_Water;
    else if ( Height < 0.2f ) return Biome_Beach;
    else if ( Height < 0.3f ) return Biome_Forest;
    else if ( Height < 0.5f ) return Biome_Jungle;
    else if ( Height < 0.7f ) return Biome_Savannah;
    else if ( Height < 0.9f ) return Biome_Desert;
    else return Biome::Biome_Snow;
    //return -1;
}

void Biome::GetBiomeColor(r32 Height, u8 Result[3])
{
    local_persist biome_color BiomeColor = {};
    
    biome Biome = GetBiome(Height);
    
    if (Biome == Biome_Water)
    {
        Result[0] = BiomeColor.Water[0];
        Result[1] = BiomeColor.Water[1];
        Result[2] = BiomeColor.Water[2];
    }
    else if (Biome == Biome_Beach)
    {
        Result[0] = BiomeColor.Beach[0];
        Result[1] = BiomeColor.Beach[1];
        Result[2] = BiomeColor.Beach[2];
    }
    else if (Biome == Biome_Forest)
    {
        Result[0] = BiomeColor.Forest[0];
        Result[1] = BiomeColor.Forest[1];
        Result[2] = BiomeColor.Forest[2];
    }
    else if (Biome == Biome_Jungle)
    {
        Result[0] = BiomeColor.Jungle[0];
        Result[1] = BiomeColor.Jungle[1];
        Result[2] = BiomeColor.Jungle[2];
    }
    else if (Biome == Biome_Beach)
    {
        Result[0] = BiomeColor.Jungle[0];
        Result[1] = BiomeColor.Jungle[1];
        Result[2] = BiomeColor.Jungle[2];
    }
    else if (Biome == Biome_Savannah)
    {
        Result[0] = BiomeColor.Savannah[0];
        Result[1] = BiomeColor.Savannah[1];
        Result[2] = BiomeColor.Savannah[2];
    }
    else if (Biome == Biome_Desert)
    {
        Result[0] = BiomeColor.Desert[0];
        Result[1] = BiomeColor.Desert[1];
        Result[2] = BiomeColor.Desert[2];
    }
    else if (Biome == Biome_Snow)
    {
        Result[0] = BiomeColor.Snow[0];
        Result[1] = BiomeColor.Snow[1];
        Result[2] = BiomeColor.Snow[2];
    }
}


// --------------------------------------------------------------------------------------------- //
// Open Simplex Noise and Erosion Algorithms
// --------------------------------------------------------------------------------------------- //
// Make sure we are not trying to access outside of the array
inline bool
CheckBounds( int i, int j, int pos, int width, int height )
{
    switch( pos )
    {
        case 0:  return j + 1 <= width - 1;
        case 1:  return j - 1 >= 0;
        case 2:  return i + 1 <= height - 1;
        case 3:  return i - 1 >= 0;
        default: return false;
    }
}

// Find the minimum of two values
inline float
min( float x, float y )
{
    return ( x < y ) ? x : y;
}

// Find the maximum of two values
inline float
max( float x, float y )
{
    return ( x > y ) ? x : y;
}

// Given an x, y, and z coordinated noise octaves are computed and added together to adapt a noise
// map. If the passed struct has a dimension of TWODIMENSION, then the 2D noise function is called.
// Othersise, call the 3D noise function.
static float Octave( SimplexNoise::NoiseOctaveSimulation& octaveInfo, float x, float y, float z ) {
    
    float maxAmp    = 0.0f;  // intital maximum amplitude
    float amp       = 1.0f;  // initial amplitude
    float freq      = 1.0f;  // initial frequency
    float noiseCell = 0.0f;  // initial noiseCell
    //noiseCell += openSimplex.eval(x, y);
    //
    // add successively smaller, higher-frequency terms
    for( int i = 0; i < octaveInfo.NumOctaves; ++i ) {
        
        // Determine which dimension to run the algorithm with
        switch( octaveInfo.Dim ) {
            case SimplexNoise::NoiseOctaveSimulation::TWODIMENSION:
            {
                noiseCell += SimplexNoise::EvalSimplexNoise(x * freq, y * freq) * amp;
            } break;
            case SimplexNoise::NoiseOctaveSimulation::THREEDIMENSION:
            {
                noiseCell += SimplexNoise::EvalSimplexNoise(x * freq, y * freq, z * freq) * amp;
            } break;
            default: return 0.0;
        }
        
        maxAmp += amp;
        amp *= octaveInfo.Persistence;
        freq *= 2.0f;
    }
    //
    // take the average value of the iterations
    noiseCell /= maxAmp;
    
    // normalize the result
    noiseCell = noiseCell * ( octaveInfo.High - octaveInfo.Low) * 0.5f
        + (octaveInfo.High + octaveInfo.Low) * 0.5f;
    return noiseCell;
}

// Runs a noise simulation using the passed struct. The size of the returned float array
// are the dimensions passed in the struct: width : height.
float* SimplexNoise::SimulateNoise( NoiseOctaveSimulation& octaveInfo) {
    
    float nx, ny, nz, noise = 0.f;
    
    //auto *noisemap = new float[ octaveInfo.width * octaveInfo.height ]{ 0 };
    r32 *noisemap = palloc<r32>(octaveInfo.Allocator, octaveInfo.Width * octaveInfo.Height);
    for (int i = 0; i < octaveInfo.Width * octaveInfo.Height; ++i)
        noisemap[i] = 0.0f;
    
    for ( int i = 0; i < octaveInfo.Height; i++ ) {
        for (int j = 0; j < octaveInfo.Width; j++) {
            //
            // Get the pixel coordinated from 0..1. Offset the value by -0.5
            // (you get problems if you don't do this)
            switch (octaveInfo.Dim) {
                // TODO Implement nz (?)
                case SimplexNoise::NoiseOctaveSimulation::TWODIMENSION:
                {
                    nx = (static_cast<float>(i) / (static_cast<float>(octaveInfo.Width) / 2)) - 0.5f;
                    ny = (static_cast<float>(j) / (static_cast<float>(octaveInfo.Height) / 2)) - 0.5f;
                    // Run octaves over the noise
                    //noisemap[(i * octaveInfo.width) + j] = openSimplex.eval(nx, ny);
                    noise = Octave( octaveInfo, nx, ny, 0 );
                } break;
                
                case SimplexNoise::NoiseOctaveSimulation::THREEDIMENSION:
                {
                    nx = (static_cast<float>(i) / (static_cast<float>(octaveInfo.Width) / 2)) - 0.5f;
                    nz = (static_cast<float>(j) / (static_cast<float>(octaveInfo.Height) / 2)) - 0.5f;
                    // Run octaves over the noise
                    noise = Octave(octaveInfo, nx, 0, nz);
                } break;
                
                default: break;
            }
            
            noisemap[(i * octaveInfo.Width) + j] = pow( noise, octaveInfo.Exp );
            //
        }
    }
    
    return noisemap;
}

// --------------------------------------------------------------------------------------------- //
// Thermal Erosion Simulation
// --------------------------------------------------------------------------------------------- //
// Run a thermal erosion simulation over a heightmap
void Erosion::SimulateThermalErosion( ThermalErosionSimlation& thermalSim )
{
    // Exit condition
    if ( thermalSim.NumberIterations <= 0 )
        return;
    
    // talos angle : orig 4 / ...
    float T = 4.0f / static_cast<float>(thermalSim.Width);
    // amount of material to "move" from the cell in question
    //float c = 0.5f;
    
    // Array for hashing
    int indexHash[] = { 1, -1, thermalSim.Width, -thermalSim.Width };
    
    for( int i = 0; i < thermalSim.Height; ++i )
    {
        for( int j = 0; j < thermalSim.Width; ++j )
        {
            
            float dmax = 0.0f;
            //float dtotal = 0.0f;
            
            float h = thermalSim.NoiseMap[ i * thermalSim.Width + j ];
            float d[] = { 0, 0, 0, 0 };
            // TODO variable not originally here
            int hli = -1;
            //float hl = 1.0f;
            
            // If there are multiple occurrences of of needed to apply erosion, find a proportion
            // based on the surrounding cells
            for( int k = 0; k < 4; ++k )
            {
                // do not check this neighbor if it is not within bounds
                if( !CheckBounds( i, j, k, thermalSim.Width, thermalSim.Height ) )
                { d[ k ] = -1; continue; }
                
                // Calculate the height difference
                d[ k ] = h - thermalSim.NoiseMap[ (i * thermalSim.Width) + j + indexHash[ k ] ];
                // TODO new implementation
                if ( d[k] > dmax )
                {
                    dmax = d[k];
                    hli = (i * thermalSim.Width) + j + indexHash[k];
                }
                // TODO original implementation
                //                if( d[ k ] > T && d[ k ] > 0 )
                //                {
                //                    dtotal += d[ k ];
                //                    if( d[ k ] > dmax ) {
                //                        dmax = d[k];
                //                        // TODO if statement not originally here
                //                        if ( d[k] < hl ) {
                //                            hli = (i * thermalSim.width) + j + indexHash[k];
                //                            hl = d[k];
                //                        }
                //                    }
                //                }
                else
                    d[ k ] = -1.0f;
            }
            
            // Now do it all again, but this time with feeling
            // Calculate the amount of material to move to each cell
            //            for( int k = 0; k < 4; ++k )
            //            {
            //                // If di = -1 then erosion should not happen here
            //                if( d[ k ] == -1 ) continue;
            //                // TODO originally no if statement
            //                if ( dmax > 0 && dmax > T ) {
            //                    thermalSim.noiseMap[(i * thermalSim.width) + j + indexHash[k]] +=
            //                            c * (dmax - T) * (d[k] / dtotal);
            //            }
            
            // TODO added implementation
            if ( hli >= 0 && dmax > 0 && dmax > T ) {
                float dh = 0.5f * dmax;
                thermalSim.NoiseMap[ (i * thermalSim.Width) + j ] -= dh;
                thermalSim.NoiseMap[ hli ] += dh;
            }
        }
    }
    
    // Recursively call thermal erosion until it finishes the number of iterations
    --thermalSim.NumberIterations;
    SimulateThermalErosion( thermalSim );
}

// --------------------------------------------------------------------------------------------- //
// Inverse Thermal Erosion Simulation
// --------------------------------------------------------------------------------------------- //
// Run a thermal erosion simulation over a heightmap
void Erosion::SimulateInverseThermalErosion( ThermalErosionSimlation& thermalSim )
{
    // Exit condition
    if ( thermalSim.NumberIterations <= 0 )
        return;
    
    // talos angle: Orig 4 / ..
    float T = 10.0f / static_cast<float>(thermalSim.Width);
    // amount of material to "move" from the cell in question
    //float c = 0.5f;
    
    // Array for hashing
    int indexHash[] = { 1, -1, thermalSim.Width, -thermalSim.Width };
    
    for( int i = 0; i < thermalSim.Height; ++i )
    {
        for( int j = 0; j < thermalSim.Width; ++j )
        {
            
            float dmax = 0.0f;
            //float height = 0.0f;
            
            float h = thermalSim.NoiseMap[ i * thermalSim.Width + j ];
            float d[] = { 0, 0, 0, 0 };
            // TODO variable not originally here
            int hli = -1;
            //float hl = 1.0f;
            
            // If there are multiple occurrences of of needed to apply erosion, find a proportion
            // based on the surrounding cells
            for( int k = 0; k < 4; ++k )
            {
                // do not check this neighbor if it is not within bounds
                if( !CheckBounds( i, j, k, thermalSim.Width, thermalSim.Height ) )
                { d[ k ] = -1; continue; }
                
                // Calculate the height difference
                d[ k ] = h - thermalSim.NoiseMap[ (i * thermalSim.Width) + j + indexHash[ k ] ];
                // TODO new implementation
                if ( d[k] > dmax )
                {
                    dmax = d[k];
                    hli = (i * thermalSim.Width) + j + indexHash[k];
                }
                // TODO original implementation
                //                if( d[ k ] > T && d[ k ] > 0 )
                //                {
                //                    dtotal += d[ k ];
                //                    if( d[ k ] > dmax ) {
                //                        dmax = d[k];
                //                        // TODO if statement not originally here
                //                        if ( d[k] < hl ) {
                //                            hli = (i * thermalSim.width) + j + indexHash[k];
                //                            hl = d[k];
                //                        }
                //                    }
                //                }
                else
                    d[ k ] = -1.0f;
            }
            
            // Now do it all again, but this time with feeling
            // Calculate the amount of material to move to each cell
            //            for( int k = 0; k < 4; ++k )
            //            {
            //                // If di = -1 then erosion should not happen here
            //                if( d[ k ] == -1 ) continue;
            //                // TODO originally no if statement
            //                if ( dmax > 0 && dmax > T ) {
            //                    thermalSim.noiseMap[(i * thermalSim.width) + j + indexHash[k]] +=
            //                            c * (dmax - T) * (d[k] / dtotal);
            //            }
            
            // TODO added implementation
            if ( hli >= 0 && dmax > 0 && dmax <= T ) {
                float dh = 0.5f * dmax;
                thermalSim.NoiseMap[ (i * thermalSim.Width) + j ] -= dh;
                thermalSim.NoiseMap[ hli ] += dh;
            }
        }
    }
    
    // Recursively call thermal erosion until it finishes the number of iterations
    --thermalSim.NumberIterations;
    SimulateInverseThermalErosion( thermalSim );
}

// --------------------------------------------------------------------------------------------- //
// Hydraulic Erosion
// --------------------------------------------------------------------------------------------- //
// Reset the simulation for next use
// coefficients remain persistent across simulation
static void Reset( Erosion::HydraulicErosionSimulation &erosionStruct)
{
    pfree<r32>(erosionStruct.Allocator, erosionStruct.WaterMap);
    pfree<r32>(erosionStruct.Allocator, erosionStruct.SedimentMap);
    
    // I don't understand why this is set to nullptr?
    erosionStruct.NoiseMap = nullptr;
}

// Runs a sungle iteration of the hydraulic erosion algorithm
static void RunSimulation( Erosion::HydraulicErosionSimulation &erosionStruct)
{
    int size = erosionStruct.Width * erosionStruct.Height;
    for( int i = 0; i < size; ++i )
        erosionStruct.SedimentMap[i] = 0.0f;
    
    int indexHash[] = { 1, -1, erosionStruct.Width, -erosionStruct.Width };
    
    /*
     * Kr is added to each cell every iteration to simulate rain
     *     w(i,j) = w(i,j) + Kr    <- constant amount of water
     *
     * Amount of the height value proportional to the amount of water present in the same
     * cell is converted to sediment.
     *    h(i,j) = h(i,j) − Ks * w(i,j)    <- Height map, solubility constant of the terrain
     *    m(i,j) = m(i,j) + Ks * w(i,j)    <- Sediment map
     */
    for( int i = 0; i < size; ++i )
    {
        erosionStruct.WaterMap[ i ] += erosionStruct.ErosionCoeffStruct.Kr;
        erosionStruct.NoiseMap[ i ] -= erosionStruct.ErosionCoeffStruct.Ks
            * erosionStruct.WaterMap[ i ];
        erosionStruct.SedimentMap[ i ] += erosionStruct.ErosionCoeffStruct.Ks
            * erosionStruct.WaterMap[ i ];
    }
    
    // Set of variables for the set part of calculations. Their explanation is above
    float atot = 0.f, dtotal = 0.f, mmax, dm, wi;
    
    float acc;
    
    // Update the percentage of water evaportated
    
    for( int i = 0; i < erosionStruct.Height; ++i )
    {
        for( int k = 0; k < erosionStruct.Width; ++k ) {
            
            int acount = 0;
            float d[] = { -1.f, -1.f, -1.f, -1.f };
            float a[] = { -1.f, -1.f, -1.f, -1.f };
            
            // height at the current cell
            float h = erosionStruct.NoiseMap[ (i * erosionStruct.Width) + k ];
            acc = erosionStruct.WaterMap[ (i * erosionStruct.Width) + k ];
            
            // Get the total height of the neighboring cells, if they are lower than the current cell
            for (int j = 0; j < 4; ++j) {
                
                if (!CheckBounds(i, k, j, erosionStruct.Width, erosionStruct.Height ) ) { continue; }
                
                // Do not run erosion if a neighboring cell is taller than the current one
                if ( erosionStruct.NoiseMap[ (i * erosionStruct.Width) + k + indexHash[ j ] ] > h) continue;
                
                ++acount;
                a[j] = erosionStruct.NoiseMap[ (i * erosionStruct.Width) + k + indexHash[ j ] ]
                    + erosionStruct.WaterMap[ (i * erosionStruct.Width) + k + indexHash[ j ] ];
                atot += a[ j ];
                
                d[ j ] = acc - a[ j ];
                if (d[j] > 0)
                    dtotal += d[ j ];
            }
            
            // ∆a = a−a ̄ is the total height of the current cell minus the average total height of the
            // cells involved in the distribution
            // a[j] - (atot/acount)
            
            // move the sediment downhill
            for (int j = 0; j < 4; ++j) {
                
                if (!CheckBounds(i, k, j, erosionStruct.Width, erosionStruct.Height ) ) { continue; }
                
                if (d[j] <= 0 || a[ j ] <= 0 )
                    continue;
                
                wi = min( erosionStruct.WaterMap[ (i * erosionStruct.Width) + k + indexHash[ j ] ],
                         a[j] - (atot/acount) ) * ( d[ j ] / dtotal );
                
                erosionStruct.NoiseMap[ (i * erosionStruct.Width) + k + indexHash[ j ] ] =
                    erosionStruct.SedimentMap[ (i * erosionStruct.Width) + k ]
                    * ( wi / erosionStruct.WaterMap[ (i * erosionStruct.Width) + k ] );
            }
            
            // Reset the current cell's height based on the amount of erosion
            erosionStruct.WaterMap[(i * erosionStruct.Width) + k]
                *= (1 - erosionStruct.ErosionCoeffStruct.Ke);
            mmax = erosionStruct.ErosionCoeffStruct.Kc *
                erosionStruct.WaterMap[ (i * erosionStruct.Width) + k ];
            dm = max( 0.f, erosionStruct.SedimentMap[ (i * erosionStruct.Width) + k ] - mmax );
            erosionStruct.WaterMap[ (i * erosionStruct.Width) + k ] -= dm;
            erosionStruct.NoiseMap[ (i * erosionStruct.Width) + k ] += dm;
        }
    }
}

// This is done for encapsulation. Don't worry about it. Starts the hydraulic erosion simulation
void Erosion::SimulateHydraulicErosion( HydraulicErosionSimulation &erosionStruct )
{
    // Initialize the variables for the simulation
    i32 size = erosionStruct.Width * erosionStruct.Height;
    
    erosionStruct.WaterMap    = palloc<r32>(erosionStruct.Allocator, size);
    erosionStruct.SedimentMap = palloc<r32>(erosionStruct.Allocator, size);
    for (i32 i = 0; i < size; ++i)
        erosionStruct.WaterMap[i] = 0.0f;
    
    // Run the erosion simulation
    for(int i = 0; i < erosionStruct.NumberIterations; ++i)
        RunSimulation(erosionStruct);
    
    // call reset
    Reset(erosionStruct);
}
