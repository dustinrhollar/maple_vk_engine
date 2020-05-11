#ifndef SPLICER_EXT_RESOURCE_MANAGER_FILE_H
#define SPLICER_EXT_RESOURCE_MANAGER_FILE_H

struct FileBuffer
{
    char *start;
    char *brkp;
    u32 cap;
};

void CreateFileBuffer(FileBuffer *buffer, size_t size = 128);
void DestroyFileBuffer(FileBuffer *buffer);
void ResizeFileBuffer(FileBuffer *buffer, u32 new_cap);
i32 BufferUnusedSize(FileBuffer *buffer);

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
u64 StrToUInt64(char *str, char *stop);
r32 ReadFloatFromString(char *buffer);
jstring UInt64ToStr(u64 data);

void UInt16ToBinaryBuffer(FileBuffer *buffer, u16 *data, u32 count);
void UInt32ToBinaryBuffer(FileBuffer *buffer, u32 *data, u32 count);
void UInt64ToBinaryBuffer(FileBuffer *buffer, u64 *data, u32 count);
void CharToBinaryBuffer(FileBuffer *buffer, const char *data, u32 count);
void BoolToBinaryBuffer(FileBuffer *buffer, bool *data, u32 count);
void JStringToBinaryBuffer(FileBuffer *buffer, jstring &str);
void FloatToBinaryBuffer(FileBuffer *buffer, r32 *data, u32 count);

void ReadFloatFromBinaryBuffer(FileBuffer *buffer, r32 *result);
void ReadBoolFromBinaryBuffer(FileBuffer *buffer, bool *result);
void ReadInt32FromBinaryBuffer(FileBuffer *buffer, i32 *result);
void ReadInt64FromBinaryBuffer(FileBuffer *buffer, i64 *result);
void ReadUInt16FromBinaryBuffer(FileBuffer *buffer, u16 *result);
void ReadUInt32FromBinaryBuffer(FileBuffer *buffer, u32 *result);
void ReadUInt64FromBinaryBuffer(FileBuffer *buffer, u64 *result);
void ReadJStringFromBinaryBuffer(FileBuffer *buffer, jstring *result);
void ReadFloatFromBinaryBuffer(FileBuffer *buffer, r32 *result);

#endif //SPLICER_EXT_RESOURCE_MANAGER_FILE_H
