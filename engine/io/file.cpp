
// TODO(Dustin):
// - Should I worry about byte order of binary files?
// - UTF-16 Support
// - Handle endian-ness of text files
// - String to floating point

//~ The following functions attempts to resolve the endian-ness of
// the target machine. Machines that default to big endian when
// writing are converted to little endian format

// Determine is a system will be big endian or little endian
// Little Endian: 01 00 00 00
// Big Endian:    00 00 00 01
const int i = 1;
#define is_bigendian() ( (*(char*)&i) == 0 )

// i16
short ReverseInt16(i16 s)
{
    // u8
    u8 c1, c2;
    
    if (is_bigendian())
    {
        c1 = s & 255;
        c2 = (s>>8) & 255;
        
        s = (c1<<8)+c2;
    }
    
    return s;
}

i32 ReverseInt32(i32 i)
{
    u8 c1, c2, c3, c4;
    
    if (is_bigendian())
    {
        c1 = ( i>>0 )  & 255;
        c2 = ( i>>8 )  & 255;
        c3 = ( i>>16 ) & 255;
        c4 = ( i>>24 ) & 255;
        
        i = ((int)c1 << 24) + ((int)c2 << 16) +  ((int)c3 << 8) + ((int)c4 << 0);
    }
    
    return i;
}

i64 ReverseInt64(i64 i)
{
    u8 c1, c2, c3, c4, c5, c6, c7, c8;
    
    if (is_bigendian())
    {
        c1 = ( i>>0 )  & 255;
        c2 = ( i>>8 )  & 255;
        c3 = ( i>>16 ) & 255;
        c4 = ( i>>24 ) & 255;
        c5 = ( i>>32 ) & 255;
        c6 = ( i>>40 ) & 255;
        c7 = ( i>>48 ) & 255;
        c8 = ( i>>56 ) & 255;
        
        i = ((i64)c1 << 56) + ((i64)c2 << 48) + ((i64)c3 << 40) + ((i64)c4 << 32)
            + ((i64)c5 << 24) + ((i64)c6 << 16) + ((i64)c7 << 8) + ((i64)c8 << 0);
    }
    
    return i;
}

u16 ReverseUInt16(u16 i)
{
    u8 c1, c2;
    
    if (is_bigendian())
    {
        c1 = ( i>>0 )  & 255;
        c2 = ( i>>8 )  & 255;
        
        i = ((u16)c1 << 24) + ((u16)c2 << 16);
    }
    
    return i;
}

u32 ReverseUInt32(u32 i)
{
    u8 c1, c2, c3, c4;
    
    if (is_bigendian())
    {
        c1 = ( i>>0 )  & 255;
        c2 = ( i>>8 )  & 255;
        c3 = ( i>>16 ) & 255;
        c4 = ( i>>24 ) & 255;
        
        i = ((u32)c1 << 24) + ((u32)c2 << 16) + ((u32)c3 << 8) + ((u32)c4 << 0);
    }
    
    return i;
}

u64 ReverseUInt64(u64 i)
{
    u8 c1, c2, c3, c4, c5, c6, c7, c8;
    
    if (is_bigendian())
    {
        c1 = ( i>>0 )  & 255;
        c2 = ( i>>8 )  & 255;
        c3 = ( i>>16 ) & 255;
        c4 = ( i>>24 ) & 255;
        c5 = ( i>>32 ) & 255;
        c6 = ( i>>40 ) & 255;
        c7 = ( i>>48 ) & 255;
        c8 = ( i>>56 ) & 255;
        
        i = ((u64)c1 << 56) + ((u64)c2 << 48) + ((u64)c3 << 40) + ((u64)c4 << 32)
            + ((u64)c5 << 24) + ((u64)c6 << 16) + ((u64)c7 << 8) + ((u64)c8 << 0);
    }
    
    return i;
}

float ReverseFloat(float f)
{
    u8 c1, c2, c3, c4;
    
    if (is_bigendian())
    {
        c1 = ( i>>0 )  & 255;
        c2 = ( i>>8 )  & 255;
        c3 = ( i>>16 ) & 255;
        c4 = ( i>>24 ) & 255;
        
        f = ((u32)c1 << 24) + ((u32)c2 << 16) + ((u32)c3 << 8) + ((u32)c4 << 0);
    }
    
    return f;
}

i16 ReadInt16(char *buffer)
{
    // a uint is just 4 bytes
    u8 c1, c2;
    
    c1 = buffer[0];
    c2 = buffer[1];
    
    // Files are written in little-endian format no matter the endian-ness of the machine
    i16 result = ((i16)c2 << 8) + ((i16)c1 << 0);
    
    return result;
}

i32 ReadInt32(char *buffer)
{
    // a uint is just 4 bytes
    u8 c1, c2, c3, c4;
    
    c1 = buffer[0];
    c2 = buffer[1];
    c3 = buffer[2];
    c4 = buffer[3];
    
    // Files are written in little-endian format no matter the endian-ness of the machine
    i32 result = ((i32)c4 << 24) + ((i32)c3 << 16) +  ((i32)c2 << 8) + ((i32)c1 << 0);
    
    return result;
}

i64 ReadInt64(char *buffer)
{
    // a uint is just 4 bytes
    u8 c1, c2, c3, c4, c5, c6, c7, c8;
    
    c1 = buffer[0];
    c2 = buffer[1];
    c3 = buffer[2];
    c4 = buffer[3];
    c5 = buffer[4];
    c6 = buffer[5];
    c7 = buffer[6];
    c8 = buffer[7];
    
    // Files are written in little-endian format no matter the endian-ness of the machine
    i64 result = ((i64)c8 << 56) + ((i64)c7 << 48) + ((i64)c6 << 40) + ((i64)c5 << 32)
        + ((i64)c4 << 24) + ((i64)c3 << 16) +  ((i64)c2 << 8) + ((i64)c1 << 0);
    
    return result;
}

u16 ReadUInt16(char *buffer)
{
    // a uint is just 4 bytes
    u8 c1, c2;
    
    c1 = buffer[0];
    c2 = buffer[1];
    
    // Files are written in little-endian format no matter the endian-ness of the machine
    u16 result = ((u16)c2 << 8) + ((u16)c1 << 0);
    
    return result;
}

u32 ReadUInt32(char *buffer)
{
    // a uint is just 4 bytes
    u8 c1, c2, c3, c4;
    
    c1 = buffer[0];
    c2 = buffer[1];
    c3 = buffer[2];
    c4 = buffer[3];
    
    // Files are written in little-endian format no matter the endian-ness of the machine
    u32 result = ((u32)c4 << 24) + ((u32)c3 << 16) +  ((u32)c2 << 8) + ((u32)c1 << 0);
    
    return result;
}

u64 ReadUInt64(char *buffer)
{
    // a uint is just 4 bytes
    u8 c1, c2, c3, c4, c5, c6, c7, c8;
    
    c1 = buffer[0];
    c2 = buffer[1];
    c3 = buffer[2];
    c4 = buffer[3];
    c5 = buffer[4];
    c6 = buffer[5];
    c7 = buffer[6];
    c8 = buffer[7];
    
    // Files are written in little-endian format no matter the endian-ness of the machine
    u64 result = ((u64)c8 << 56) + ((u64)c7 << 48) + ((u64)c6 << 40) + ((u64)c5 << 32)
        + ((u64)c4 << 24) + ((u64)c3 << 16) +  ((u64)c2 << 8) + ((u64)c1 << 0);
    
    return result;
}

bool ReadBool(char *buffer)
{
    bool result = (bool)buffer[0];
    return result;
}

float ReadFloat(char *buffer)
{
    // a uint is just 4 bytes
    u8 c1, c2, c3, c4;
    
    c1 = buffer[0];
    c2 = buffer[1];
    c3 = buffer[2];
    c4 = buffer[3];
    
    // Files are written in little-endian format no matter the endian-ness of the machine
    u32 result = ((u32)c4 << 24) + ((u32)c3 << 16) +  ((u32)c2 << 8) + ((u32)c1 << 0);
    return *((float*) &result);
}

// does not contain the sign
int StrToInt(char *str, char *stop)
{
    int result = 0;
    for (char *c = str; c < stop; ++c)
    {
        result = result * 10 + c[0] - '0';
    }
    
    return result;
}

// does not contain the sign
i64 StrToInt64(char *str, char *stop)
{
    i64 result = 0;
    for (char *c = str; c < stop; ++c)
    {
        result = result * 10 + c[0] - '0';
    }
    
    return result;
}

u64 StrToUInt64(char *str, char *stop)
{
    u64 result = 0;
    for (char *c = str; c < stop; ++c)
    {
        result = result * 10 + c[0] - '0';
    }
    
    return result;
}

// Takes in a string like: "127.8"
// and converts it to a floating point number
r32 ReadFloatFromString(char *buffer)
{
    return (r32)strtod(buffer, NULL);
}

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
jstring UInt64ToStr(u64 value)
{
    // max of u64 is something like 20 digits
    char result_array[20];
    int base = 10;
    
    char* ptr = result_array, *ptr1 = result_array, tmp_char;
    u64 tmp_value;
    
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );
    
    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    
    jstring result;
    InitJString(&result, result_array);
    return result;
}

//~ File I/O Code
void CreateFileBuffer(FileBuffer *buffer, size_t size)
{
    // create a buffer with 128 bytes
    buffer->start = (char*)palloc(size);
    buffer->brkp  = buffer->start;
    buffer->cap   = size;
}

void ResizeFileBuffer(FileBuffer *buffer, u32 new_cap)
{
    u32 offset = buffer->brkp - buffer->start;
    
    buffer->start = (char*)prealloc(buffer->start, new_cap);
    buffer->brkp  = buffer->start + offset;
    buffer->cap   = new_cap;
}

inline bool NeedsResize(FileBuffer *buffer, u32 req_size)
{
    return buffer->brkp + req_size > buffer->start + buffer->cap;
}

inline void ResizeIfPossible(FileBuffer *buffer, u32 req_size)
{
    if (NeedsResize(buffer, req_size))
        ResizeFileBuffer(buffer, (buffer->cap * 2 > req_size) ? buffer->cap * 2 : req_size * 2);
}

//~ Write Routines
void Int32ToBinaryBuffer(FileBuffer *buffer, i32 *data, u32 count)
{
    u32 req_size = count * sizeof(i32);
    ResizeIfPossible(buffer, req_size);
    
    // if, for whatever reason, the system is big endian,
    // reverse the byte order
    for (u32 i = 0; i < count; ++i)
        data[i] = ReverseInt32(data[i]);
    
    memcpy(buffer->brkp, data, req_size);
    
    buffer->brkp += req_size;
}

void UInt16ToBinaryBuffer(FileBuffer *buffer, u16 *data, u32 count)
{
    u32 req_size = count * sizeof(u16);
    ResizeIfPossible(buffer, req_size);
    
    // if, for whatever reason, the system is big endian,
    // reverse the byte order
    for (u32 i = 0; i < count; ++i)
        data[i] = ReverseUInt16(data[i]);
    
    memcpy(buffer->brkp, data, req_size);
    
    buffer->brkp += req_size;
}

void UInt32ToBinaryBuffer(FileBuffer *buffer, u32 *data, u32 count)
{
    u32 req_size = count * sizeof(u32);
    ResizeIfPossible(buffer, req_size);
    
    // if, for whatever reason, the system is big endian,
    // reverse the byte order
    for (u32 i = 0; i < count; ++i)
        data[i] = ReverseUInt32(data[i]);
    
    memcpy(buffer->brkp, data, req_size);
    
    buffer->brkp += req_size;
}

void UInt64ToBinaryBuffer(FileBuffer *buffer, u64 *data, u32 count)
{
    u32 req_size = count * sizeof(u64);
    ResizeIfPossible(buffer, req_size);
    
    // if, for whatever reason, the system is big endian,
    // reverse the byte order
    for (u32 i = 0; i < count; ++i)
        data[i] = ReverseUInt64(data[i]);
    
    memcpy(buffer->brkp, data, req_size);
    
    buffer->brkp += req_size;
}

void Int64ToBinaryBuffer(FileBuffer *buffer, i64 *data, u32 count)
{
    u32 req_size = count * sizeof(i64);
    ResizeIfPossible(buffer, req_size);
    
    // if, for whatever reason, the system is big endian,
    // reverse the byte order
    for (u32 i = 0; i < count; ++i)
        data[i] = ReverseInt64(data[i]);
    
    memcpy(buffer->brkp, data, req_size);
    
    buffer->brkp += req_size;
}

void CharToBinaryBuffer(FileBuffer *buffer, const char *data, u32 count)
{
    u32 req_size = count * sizeof(char);
    ResizeIfPossible(buffer, req_size);
    
    memcpy(buffer->brkp, data, req_size);
    buffer->brkp += req_size;
}

void BoolToBinaryBuffer(FileBuffer *buffer, bool *data, u32 count)
{
    u32 req_size = count * sizeof(bool);
    ResizeIfPossible(buffer, req_size);
    
    memcpy(buffer->brkp, data, req_size);
    buffer->brkp += req_size;
}

void JStringToBinaryBuffer(FileBuffer *buffer, jstring &str)
{
    // Writes in the format:
    // flags
    // character array
    u32 flags = (str.len<<1) | (str.heap);
    UInt32ToBinaryBuffer(buffer, &flags, 1);
    
    CharToBinaryBuffer(buffer, str.GetCStr(), str.len);
}

void FloatToBinaryBuffer(FileBuffer *buffer, r32 f)
{
    // assume little endian value
    u32 req_size = sizeof(r32);
    ResizeIfPossible(buffer, req_size);
    
    char *mem = buffer->brkp;
    buffer->brkp += req_size;
    
    // just in case we are on a big-endian system
    f = ReverseFloat(f);
    memcpy(mem, &f, sizeof(r32));
}

void EntityToBinaryBuffer(FileBuffer *buffer, ecs::Entity entity)
{
    UInt64ToBinaryBuffer(buffer, &entity.id, 1);
}

// NOTE(Dustin): Prints a head for the array - not the array itself!
template<class T>
void DynamicArrayToBinaryBuffer(FileBuffer *buffer, DynamicArray<T> *array)
{
    // Simply need to print: size, cap (?)
    u32 data[] = {
        array->size,
        array->cap
    };
    UInt32ToBinaryBuffer(buffer, data, 2);
}

//~ Read Routines

void ReadFloatFromBinaryBuffer(FileBuffer *buffer, r32 *result)
{
    // assume little endian value
    u32 req_size = sizeof(r32);
    
    if (NeedsResize(buffer, req_size))
    {
        printf("Not enough space left to read jstring from buffer!\n");
    }
    else
    {
        *result = ReadFloat(buffer->brkp);
        buffer->brkp += sizeof(r32);
    }
}

void ReadBoolFromBinaryBuffer(FileBuffer *buffer, bool *result)
{
    u32 req_size = sizeof(bool);
    if (NeedsResize(buffer, req_size))
    {
        printf("Not enough space left to read jstring from buffer!\n");
    }
    else
    {
        *result = ReadBool(buffer->brkp);
        buffer->brkp += req_size;
    }
}

void ReadInt32FromBinaryBuffer(FileBuffer *buffer, i32 *result)
{
    u32 req_size = sizeof(i32);
    
    if (NeedsResize(buffer, req_size))
    {
        printf("Not enough space left to read jstring from buffer!\n");
    }
    else
    {
        *result = ReadInt32(buffer->brkp);
        buffer->brkp += req_size;
    }
}

void ReadInt64FromBinaryBuffer(FileBuffer *buffer, i64 *result)
{
    u32 req_size = sizeof(i64);
    
    if (NeedsResize(buffer, req_size))
    {
        printf("Not enough space left to read jstring from buffer!\n");
    }
    else
    {
        *result = ReadInt64(buffer->brkp);
        buffer->brkp += req_size;
    }
}

void ReadUInt16FromBinaryBuffer(FileBuffer *buffer, u16 *result)
{
    u32 req_size = sizeof(u16);
    
    if (NeedsResize(buffer, req_size))
    {
        printf("Not enough space left to read jstring from buffer!\n");
    }
    else
    {
        *result = ReadUInt16(buffer->brkp);
        buffer->brkp += req_size;
    }
}

void ReadUInt32FromBinaryBuffer(FileBuffer *buffer, u32 *result)
{
    u32 req_size = sizeof(u32);
    
    if (NeedsResize(buffer, req_size))
    {
        printf("Not enough space left to read jstring from buffer!\n");
    }
    else
    {
        *result = ReadUInt32(buffer->brkp);
        buffer->brkp += req_size;
    }
}

void ReadUInt64FromBinaryBuffer(FileBuffer *buffer, u64 *result)
{
    u32 req_size = sizeof(u64);
    
    if (NeedsResize(buffer, req_size))
    {
        printf("Not enough space left to read jstring from buffer!\n");
    }
    else
    {
        *result = ReadInt64(buffer->brkp);
        buffer->brkp += req_size;
    }
}

void ReadEntityFromBinaryBuffer(FileBuffer *buffer, ecs::Entity *entity)
{
    ReadUInt64FromBinaryBuffer(buffer, &entity->id);
}

template<class T>
u32 ReadDynamicArrayFromBinaryBuffer(FileBuffer *buffer, DynamicArray<T> *array)
{
    // File is written to in the order of:
    // size : u32
    // cap  : u32
    u32 size;
    u32 cap;
    
    ReadUInt32FromBinaryBuffer(buffer, &size);
    ReadUInt32FromBinaryBuffer(buffer, &cap);
    
    // Construct an array just large enough to hold the data
    *array = DynamicArray<T>(size + 1);
    
    return size;
}

void ReadJStringFromBinaryBuffer(FileBuffer *buffer, jstring *result)
{
    u32 req_size = sizeof(u32);
    
    if (NeedsResize(buffer, req_size))
    {
        printf("Not enough space left to read jstring from buffer!\n");
    }
    else
    {
        u32 flags = ReadUInt32(buffer->brkp);
        buffer->brkp += sizeof(u32);
        
        u32 len = flags>>1;
        
        InitJString(result, buffer->brkp, len);
        buffer->brkp += len;
    }
}
