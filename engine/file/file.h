#ifndef MAPLE_FILE_FILE_H
#define MAPLE_FILE_FILE_H

struct FileBuffer
{
    char *start;
    char *brkp;
    u32   cap;
};

void CreateFileBuffer(FileBuffer *buffer, size_t size = 128);
void DestroyFileBuffer(FileBuffer *buffer);
void ResizeFileBuffer(FileBuffer *buffer, u32 new_cap);
void WriteToFileBuffer(FileBuffer *Buffer, char *Fmt, ...);
inline void ResizeIfPossible(FileBuffer *buffer, u32 req_size);
inline i32 BufferUnusedSize(FileBuffer *buffer);

i16 ReverseInt16(i16 s);
i32 ReverseInt32(i32 i);
i64 ReverseInt64(i64 i);
u16 ReverseUInt16(u16 i);
u32 ReverseUInt32(u32 i);
u64 ReverseUInt64(u64 i);
r32 ReverseFloat(float f);
i16 ReadInt16(char *buffer);
i32 ReadInt32(char *buffer);
u16 ReadUInt16(char *buffer);
u32 ReadUInt32(char *buffer);
u64 ReadUInt64(char *buffer);
r32 ReadFloat(char *buffer);
i32 StrToInt(char *str, char *stop);
i64 StrToInt64(char *str, char *stop);
u32 StrToUInt(char *str, char *stop);
u64 StrToUInt64(char *str, char *stop);
r32 StrToR32(char *buffer);
r64 StrToR64(char *buffer);
mstring UInt64ToStr(u64 data);

bool CharToBinaryBuffer(FileBuffer *buffer, const char *data, u32 count);
bool Int32ToBinaryBuffer(FileBuffer *buffer, i32 *data, u32 count);
bool UInt16ToBinaryBuffer(FileBuffer *buffer, u16 *data, u32 count);
bool UInt32ToBinaryBuffer(FileBuffer *buffer, u32 *data, u32 count);
bool UInt64ToBinaryBuffer(FileBuffer *buffer, u64 *data, u32 count);
bool CharToBinaryBuffer(FileBuffer *buffer, const char *data, u32 count);
bool BoolToBinaryBuffer(FileBuffer *buffer, bool *data, u32 count);
bool JStringToBinaryBuffer(FileBuffer *buffer, mstring &str);
bool FloatToBinaryBuffer(FileBuffer *buffer, r32 *data, u32 count);

// TODO(Dustin): Allow for reading more than one value at a time
void ReadFloatFromBinaryBuffer(FileBuffer *buffer, r32 *result);
void ReadBoolFromBinaryBuffer(FileBuffer *buffer, bool *result);
void ReadInt32FromBinaryBuffer(FileBuffer *buffer, i32 *result);
void ReadInt64FromBinaryBuffer(FileBuffer *buffer, i64 *result);
void ReadUInt16FromBinaryBuffer(FileBuffer *buffer, u16 *result);
void ReadUInt32FromBinaryBuffer(FileBuffer *buffer, u32 *result);
void ReadUInt64FromBinaryBuffer(FileBuffer *buffer, u64 *result);
void ReadMstringFromBinaryBuffer(FileBuffer *buffer, mstring *result);
void ReadFloatFromBinaryBuffer(FileBuffer *buffer, r32 *result);

#endif //MAPLE_FILE_FILE_H
