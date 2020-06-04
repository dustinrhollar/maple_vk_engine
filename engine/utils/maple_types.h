#ifndef MAPLE_TYPES
#define MAPLE_TYPES

#include <stdint.h>

#define file_internal static
#define file_global   static
#define local_persist static

#define i8  int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define r32 float
#define r64 double

#define uptr uintptr_t
#define iptr intptr_t

typedef union
{
    struct { i64 upper, lower; };
    i64 bits[2];
} u128;

// Defines for calculating size
#define _KB(x) (x * 1024)
#define _MB(x) (_KB(x) * 1024)
#define _GB(x) (_MB(x) * 1024)

#define _64KB  _KB(64)
#define _1MB   _MB(1)
#define _2MB   _MB(2)
#define _4MB   _MB(4)
#define _8MB   _MB(8)
#define _16MB  _MB(16)
#define _32MB  _MB(32)
#define _64MB  _MB(64)
#define _128MB _MB(128)
#define _256MB _MB(256)
#define _1GB   _GB(1)

#define EID u64

#define BIT(x) (1<<(x))

#endif
