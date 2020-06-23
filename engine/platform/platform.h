#ifndef MAPLE_PLATFORM_PLATFORM_H
#define MAPLE_PLATFORM_PLATFORM_H

// Platform Memory allocation
void* PlatformRequestMemory(u64 Size);
void PlatformReleaseMemory(void *Ptr, u64 Size);

// Log/Printing


// File I/O


// Timing
u64 PlatformGetWallClock();
r32 PlatformGetElapsedSeconds(u64 Start, u64 End);

// Bit shifting
__inline u32 PlatformClzl(u64 Value);
u32 PlatformCtzl(u64 Value);

#endif //MAPLE_PLATFORM_PLATFORM_H
