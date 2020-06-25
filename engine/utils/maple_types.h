#ifndef MAPLE_UTILS_MAPLE_TYPES_H
#define MAPLE_UTILS_MAPLE_TYPES_H

#include <stdint.h>

#define file_global   static
#define file_internal static
#define local_persist static

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  r32;
typedef double r64;

typedef uintptr_t uptr;
typedef intptr_t iptr;

typedef union
{
    struct { i64 Upper, Lower; };
    i64 Bits[2];
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

#define BIT(x) 1<<(x)

#endif //MAPLE_UTILS_MAPLE_TYPES_H
