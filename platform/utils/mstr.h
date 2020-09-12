/* date = August 13th 2020 6:42 pm */

#ifndef ENGINE_UTILS_MSTR_H
#define ENGINE_UTILS_MSTR_H

typedef union 
{
    struct
    {
        char Stack[12];
        u32       Len;
    };
    
    struct
    {
        char *Heap;
        u32   HeapSize;
    };
    
} mstr;

mstr mstr_init(char *Cstr, u32 CStrLen);
void mstr_free(mstr *String);
mstr mstr_add(mstr *Left, mstr *Right);
mstr mstr_concat(mstr *Src, mstr *Other);
mstr cstr_concat(mstr *Src, char *Other);
mstr cstr_add(char *Left, u32 LeftLen, char *Right, u32 RightLen);
char *mstr_to_cstr(mstr *Src);

#endif //MSTR_H

#if defined(USE_MAPLE_MSTR_IMPLEMENTATION)

mstr mstr_init(char *Cstr, u32 CstrLen)
{
    mstr Result = {0};
    Result.Len = CstrLen;
    
    if (Cstr && CstrLen > 0)
    {
        if (CstrLen > 11)
        {
            Result.HeapSize = CstrLen + 1;
            Result.Heap = (char*)memory_alloc(Core->Memory, CstrLen + 1);
            memcpy(Result.Heap, Cstr, Result.Len);
            Result.Heap[Result.Len] = 0;
        }
        else
        {
            memcpy(Result.Stack, Cstr, Result.Len);
            Result.Stack[Result.Len] = 0;
        }
    }
    
    return Result;
}

void mstr_free(mstr *String)
{
    if (String->Len > 11) 
    {
        memory_release(Core->Memory, String->Heap);
        String->Heap = NULL;
        String->HeapSize = 0;
    }
    
    String->Len = 0;
}

mstr mstr_add(mstr *Left, mstr *Right)
{
    mstr Result = {0};
    Result.Len = Left->Len + Right->Len;
    
    if (Result.Len > 11)
    {
        if (Result.Len > 11)
        {
            Result.HeapSize = Result.Len + 1;
            Result.Heap = (char*)memory_alloc(Core->Memory, Result.HeapSize);
            
            memcpy(Result.Heap, mstr_to_cstr(Left), Left->Len);
            memcpy(Result.Heap + Left->Len, mstr_to_cstr(Right), Right->Len);
            
            Result.Heap[Result.Len] = 0;
        }
        else
        {
            memcpy(Result.Stack, mstr_to_cstr(Left), Left->Len);
            memcpy(Result.Stack + Left->Len, mstr_to_cstr(Right), Right->Len);
            Result.Stack[Result.Len] = 0;
        }
    }
    
    return Result;
}

mstr mstr_concat(mstr *Src, mstr *Other)
{
    mstr Result = {0};
    
    return Result;
}

mstr cstr_concat(mstr *Src, char *Other)
{
    mstr Result = {0};
    
    return Result;
}

mstr cstr_add(char *Left, u32 LeftLen, char *Right, u32 RightLen)
{
    mstr Result = {0};
    Result.Len = LeftLen + RightLen;
    
    if (Result.Len > 11)
    {
        if (Result.Len > 11)
        {
            Result.HeapSize = Result.Len + 1;
            Result.Heap = (char*)memory_alloc(Core->Memory, Result.HeapSize);
            
            memcpy(Result.Heap, Left, LeftLen);
            memcpy(Result.Heap + LeftLen, Right, RightLen);
            
            Result.Heap[Result.Len] = 0;
        }
        else
        {
            memcpy(Result.Stack, Left, LeftLen);
            memcpy(Result.Stack + LeftLen, Right, RightLen);
            Result.Stack[Result.Len] = 0;
        }
    }
    
    return Result;
}

char *mstr_to_cstr(mstr *Src)
{
    return (Src->Len > 11) ? Src->Heap : Src->Stack;
}

#endif