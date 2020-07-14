//
// Created by Dustin Hollar on 10/15/18.
//

#ifndef SIMPLEX_EROSION_H
#define SIMPLEX_EROSION_H

/**
 * The Biome namespace contains information and functions regarding a Biome in the terrain.
 *
 * @enum Biome, a set of enums representing each available Biome. Here is the list of available
 *       biomes in order:
 *           WATER
 *           BEACH
 *           FOREST
 *           JUNGLE
 *           SAVANNAH
 *           DESERT
 *           SNOW
 * @struct BiomeColor sets the color of a Biome as an unsigned character. Here is the list of
 *         default colors for each biome as RGB values.
 *             Water     0x00, 0x00, 0xFF
 *             Beach     0xFF, 0xFD, 0xD0
 *             Forest    0x22, 0x8B, 0x22
 *             Jungle    0x0B, 0x2F, 0x25
 *             Savannah  0xF0, 0xE6, 0x8C
 *             Desert    0xED, 0xC9, 0xAF
 *             Snow      0xFF, 0xFF, 0xFF
 * @function biome( float ) calculates the biome at a specified height value and returns the
 *           enum of the biome.
 */
namespace Biome {
    
    enum Biome {
        WATER,
        BEACH,
        FOREST,
        JUNGLE,
        SAVANNAH,
        DESERT,
        SNOW
    };
    
    struct BiomeColor {
        unsigned char water[3]    = { 0x00, 0x00, 0xFF };
        // Pale: (255, 253, 208), 0xFF, 0xFD, 0xD0
        unsigned char beach[3]    = { 0xFF, 0xFD, 0xD0 };
        // (34,139,34), 0x22, 0x8B, 0x22
        unsigned char forest[3]   = { 0x22, 0x8B, 0x22 };
        // 11, 47, 37
        unsigned char jungle[3]   = { 0x0B, 0x2F, 0x25 };
        // (240,230,140), 0xF0, 0xE6, 0x8C
        unsigned char savannah[3] = { 0xF0, 0xE6, 0x8C };
        // (237, 201, 175)
        unsigned char desert[3]   = { 0xED, 0xC9, 0xAF };
        // white
        unsigned char snow[3]     = { 0xFF, 0xFF, 0xFF };
    };
    
    int biome( float height );
}

/**
 * SimplexNoise namespace contains information for calculating Simplex Noise.
 *
 * @struct NoiseOctaveSimulation a struct that defines a SimplexNoise simulation. The parameters
 *         are as follows:
 *             Dimension enum: specifies the dimension of noise
 *             numOctaves: number of octaves to be ran in the simulation
 *             persistence: controls the roughness of the noise
 *             low: low output value
 *             high: high output value
 *             exp: Controls the intesity from black to white noise
 *             width: width of the noise texture
 *             height: height of the noise texture
 * @enum Dimension contains  two enums representing a dimension of space:
 *           TWODIMENSION: 2D space
 *           THREEDIMENSION: 3D space
 * @function simulateNoise( NoiseOctaveSimulation& octaveInfor ) runs the Simplex Noise simulation
 *           using a passed NoiseOctaveSimulation struct.
 */
namespace SimplexNoise {
    
    // Permutation array of random values from 0-255
    /*
    const uint8_t p[] = {151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7,
        225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190,
        6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117,
        35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136,
        171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158,
        231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46,
        245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209,
        76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86,
        164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5,
        202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16,
        58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44,
        154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253,
        19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
        228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51,
        145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184,
        84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93,
        222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180 };
    */
    
    struct NoiseOctaveSimulation {
        
        free_allocator *Allocator;
        
        enum Dimension {
            TWODIMENSION,
            THREEDIMENSION
        };
        
        // Dimension being used
        Dimension Dim;
        
        // Octave Information
        i32 NumOctaves;     // Control the number of octaves used
        r32 Persistence;  // Control the roughness of the output
        r32 Low;          // Low output value
        r32 High;         // Height output value
        r32 Exp;          // Controls the intensity of black to white
        
        // Image information
        i32 Width;
        i32 Height;
        
    };
    
    r32* SimulateNoise(NoiseOctaveSimulation& octaveInfo);
}

/**
 * Contains information for two Erosion algorithms: Thermal and Hydraulic Erosion.
 *
 * @struct ThermalErosionSimlation sets the parameters for a Thermal Erosion simulation.
 *             width: width of the heightmap
 *             height: height of the heightmap
 *             numberIterations: number of times to run the algorithm
 *             noiseMpa: heightmap to run the algorithm on
 * @function simulateThermalErosion( ThermalErosionSimlation& ) runs the erosion algorithm
 *           based on the passed ThermalErosionSimlation struct.
 * @struct ErosionCoefficient a set of coefficients that are used in Hydraulic Erosion
 *             Kr: Rain constant
 *             Ks: solubility constant of the terrain
 *             Ke: evaporation coefficient
 *             Kc: sediment transfer maximum coefficient
 * @struct HydraulicErosionSimulation sets the parameters for running a Hydraulic Erosion
 *         simulation.
 *             erosionCoeffStruct: pointer to ErosionCoefficient
 *             width: width of the heightmap
 *             height: height of the heightmap
 *             numberIterations: number of times to run the erosion algorithm
 *             noiseMap: pointer to the heightmap that erosion will be run on
 * @function simulateHydraulicErosion( HydraulicErosionSimulation& ) runs a HydraulicErosion
 *           simulations using the HydraulicErosionSimulation struct as initial parameters.
 */
namespace Erosion {
    
    // --------------------------------------------------------------------------------------------- //
    // Thermal Erosion
    // --------------------------------------------------------------------------------------------- //
    struct ThermalErosionSimlation {
        i32 Width;
        i32 Height;
        // Needs to be reset for every simulation
        i32 NumberIterations;
        
        r32* NoiseMap;
    };
    
    void SimulateThermalErosion( ThermalErosionSimlation& thermalSim );
    
    void SimulateInverseThermalErosion( ThermalErosionSimlation& thermalSim );
    
    // --------------------------------------------------------------------------------------------- //
    // Hydraulic Erosion
    // --------------------------------------------------------------------------------------------- //
    struct ErosionCoefficient {
        float Kr;    // Rain constant
        float Ks;    // solubility constant of the terrain
        float Ke;    // evaporation coefficient
        float Kc;    // sediment transfer maximum coefficient
    };
    
    struct HydraulicErosionSimulation {
        free_allocator *Allocator;
        
        ErosionCoefficient ErosionCoeffStruct;
        
        int Width;
        int Height;
        int NumberIterations;
        
        r32 *NoiseMap;     // temporary noise map
        
        r32 *SedimentMap; // 2 maps that are used for the simulation
        r32 *WaterMap;    // they are allocated/freed by the algorithm
    };
    
    void SimulateHydraulicErosion( HydraulicErosionSimulation &erosionStruct );
}

#endif //SIMPLEX_EROSION_H
