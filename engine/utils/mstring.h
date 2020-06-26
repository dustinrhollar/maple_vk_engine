#ifndef MAPLE_MSTRING_H_
#define MAPLE_MSTRING_H_

#define STRING_STACK_SIZE 12

typedef union
{
    struct
    {
        char Sptr[STRING_STACK_SIZE];
        u32 Len:31;
        u32 Heap:1;
    };
    
    struct
    {
        char *Hptr;
        u32 ReservedHeapSize;
    };
} mstring;

#define GetStr(s) (((s)->Heap)?(s)->Hptr:(s)->Sptr)

// mstring is allocated from a global arena and
// can be considered to be located in "global memory"
// A tstring is a temporary string used for per-frame
// or scope local allocations.
typedef mstring tstring;

void StringArenaInit(void *Ptr, u64 Size);
void StringArenaFree();

// Pass NULL to Cstr if you want to reserve space, but not
// load a string yet
void Mstring(mstring *Str, const char *Cstr, u32 Len);
void MstringFree(mstring *Str);

void Tstring(tstring *Str, tagged_heap_block *TaggedHeap, const char *Cstr, u32 Len);

void MstringAdd(mstring *Result, const mstring *Left, const mstring *Right);
void StringAdd(mstring *Result, const char *Left, u32 LeftLen, const char *Right, u32 RightLen);

void TstringAdd(tstring *Result, tagged_heap_block *TaggedHeap, const tstring *Left, const tstring *Right);
void TaggedStringAdd(tstring *Result, tagged_heap_block *TaggedHeap, const char *Left, u32 LeftLen, const char *Right, u32 RightLen);

// String comparison that is valid for both mstring and tstring
bool StringCmp(const char *Left, u32 LeftLen, const char *Right, u32 RightLen);


#endif //MAPLE_MSTRING_H_


#ifdef MAPLE_MSTRING_IMPLEMENTATION

file_global free_allocator StringArena;

void StringArenaInit(void *Ptr, u64 Size)
{
    FreeListAllocatorInit(&StringArena, Size, Ptr);
}

void StringArenaFree()
{
    FreeListAllocatorFree(&StringArena);
}

void Mstring(mstring *Str, const char *Cstr, u32 Len)
{
    if (Cstr)
    {
        Str->Len = Len;
        
        if (Str->Len > STRING_STACK_SIZE - 1)
        {
            Str->Heap = 1;
            Str->ReservedHeapSize = Str->Len + 1;
            Str->Hptr = palloc<char>(&StringArena, Str->ReservedHeapSize);
            memcpy(Str->Hptr, Cstr, Len);
            Str->Hptr[Len] = 0;
        }
        else
        {
            Str->Heap = 0;
            memcpy(Str->Sptr, Cstr, Len);
            Str->Sptr[Len] = 0;
        }
    }
    else
    {
        Str->Len  = 0;
        Str->Heap = 0;
        
        if (Str->Len > STRING_STACK_SIZE - 1)
        {
            Str->Heap = 1;
            Str->ReservedHeapSize = Str->Len + 1;
            Str->Hptr = palloc<char>(&StringArena, Str->ReservedHeapSize);
        }
    }
}

void MstringFree(mstring *Str)
{
    if (Str->Heap)
    {
        pfree<char>(&StringArena, Str->Hptr);
        Str->Heap             = 0;
        Str->ReservedHeapSize = 0;
        Str->Hptr             = NULL;
    }
    
    Str->Len = 0;
}

void MstringAdd(mstring *Result, const mstring *Left, const mstring *Right)
{
    if (Result == Left)
    { // attempting to accumulate into the left side of the operator
        u32 NewSize = Result->Len + Right->Len;
        
        if (Result->Heap)
        {
            if (Result->ReservedHeapSize <= NewSize)
            {
                Result->ReservedHeapSize = (Result->ReservedHeapSize * 2 > NewSize) ? Result->ReservedHeapSize*2 : NewSize + 1;
                
                char *NewPtr = palloc<char>(&StringArena, Result->ReservedHeapSize);
                memcpy(NewPtr, Result->Hptr, Result->Len);
                pfree<char>(&StringArena, Result->Hptr);
                Result->Hptr = NewPtr;
            }
            
            memcpy(Result->Hptr + Left->Len, GetStr(Right), Right->Len);
            Result->Hptr[NewSize] = 0;
        }
        else if (NewSize > STRING_STACK_SIZE - 1)
        {
            Result->Heap = 1;
            Result->ReservedHeapSize = NewSize + 1;
            
            char *NewPtr = palloc<char>(&StringArena, Result->ReservedHeapSize);
            memcpy(NewPtr, Result->Sptr, Result->Len);
            Result->Hptr = NewPtr;
            
            memcpy(Result->Hptr + Left->Len, GetStr(Right), Right->Len);
            Result->Hptr[NewSize] = 0;
        }
        else
        {
            memcpy(Result->Sptr + Left->Len, Right->Sptr, Right->Len);
            Result->Sptr[NewSize] = 0;
        }
        
        Result->Len = NewSize;
    }
    else
    {
        u32 NewSize = Result->Len + Left->Len + Right->Len;
        
        if (Result->Heap)
        {
            if (Result->ReservedHeapSize <= NewSize)
            {
                Result->ReservedHeapSize = (Result->ReservedHeapSize * 2 > NewSize) ? Result->ReservedHeapSize*2 : NewSize + 1;
                
                char *NewPtr = palloc<char>(&StringArena, Result->ReservedHeapSize);
                memcpy(NewPtr, Result->Hptr, Result->Len);
                pfree<char>(&StringArena, Result->Hptr);
                Result->Hptr = NewPtr;
            }
            
            memcpy(Result->Hptr + Result->Len, GetStr(Left), Left->Len);
            memcpy(Result->Hptr + Result->Len + Left->Len, GetStr(Right), Right->Len);
            Result->Hptr[NewSize] = 0;
        }
        else if (NewSize > STRING_STACK_SIZE - 1)
        {
            Result->Heap = 1;
            Result->ReservedHeapSize = NewSize + 1;
            
            char *NewPtr = palloc<char>(&StringArena, Result->ReservedHeapSize);
            memcpy(NewPtr, Result->Sptr, Result->Len);
            Result->Hptr = NewPtr;
            
            memcpy(Result->Hptr + Result->Len, GetStr(Left), Left->Len);
            memcpy(Result->Hptr + Result->Len + Left->Len, GetStr(Right), Right->Len);
            Result->Hptr[NewSize] = 0;
        }
        else
        {
            memcpy(Result->Sptr + Result->Len, Left->Sptr, Left->Len);
            memcpy(Result->Sptr + Result->Len + Left->Len, Right->Sptr, Right->Len);
        }
        
        Result->Len = NewSize;
    }
}

void MstringAdd(mstring *Result, const mstring *Left, const char *Right, u32 RightLen)
{
    if (Result == Left)
    { // attempting to accumulate into the left side of the operator
        u32 NewSize = Result->Len + RightLen;
        
        if (Result->Heap)
        {
            if (Result->ReservedHeapSize <= NewSize)
            {
                Result->ReservedHeapSize = (Result->ReservedHeapSize * 2 > NewSize) ? Result->ReservedHeapSize*2 : NewSize + 1;
                
                char *NewPtr = palloc<char>(&StringArena, Result->ReservedHeapSize);
                memcpy(NewPtr, Result->Hptr, Result->Len);
                pfree<char>(&StringArena, Result->Hptr);
                Result->Hptr = NewPtr;
            }
            
            memcpy(Result->Hptr + Left->Len, Right, RightLen);
            Result->Hptr[NewSize] = 0;
        }
        else if (NewSize > STRING_STACK_SIZE - 1)
        {
            Result->Heap = 1;
            Result->ReservedHeapSize = NewSize + 1;
            
            char *NewPtr = palloc<char>(&StringArena, Result->ReservedHeapSize);
            memcpy(NewPtr, Result->Sptr, Result->Len);
            Result->Hptr = NewPtr;
            
            memcpy(Result->Hptr + Left->Len, Right, RightLen);
            Result->Hptr[NewSize] = 0;
        }
        else
        {
            memcpy(Result->Sptr + Left->Len, Right, RightLen);
            Result->Sptr[NewSize] = 0;
        }
        
        Result->Len = NewSize;
    }
    else
    {
        u32 NewSize = Result->Len + Left->Len + RightLen;
        
        if (Result->Heap)
        {
            if (Result->ReservedHeapSize <= NewSize)
            {
                Result->ReservedHeapSize = (Result->ReservedHeapSize * 2 > NewSize) ? Result->ReservedHeapSize*2 : NewSize + 1;
                
                char *NewPtr = palloc<char>(&StringArena, Result->ReservedHeapSize);
                memcpy(NewPtr, Result->Hptr, Result->Len);
                pfree<char>(&StringArena, Result->Hptr);
                Result->Hptr = NewPtr;
            }
            
            memcpy(Result->Hptr + Result->Len, GetStr(Left), Left->Len);
            memcpy(Result->Hptr + Result->Len + Left->Len, Right, RightLen);
            Result->Hptr[NewSize] = 0;
        }
        else if (NewSize > STRING_STACK_SIZE - 1)
        {
            Result->Heap = 1;
            Result->ReservedHeapSize = NewSize + 1;
            
            char *NewPtr = palloc<char>(&StringArena, Result->ReservedHeapSize);
            memcpy(NewPtr, Result->Sptr, Result->Len);
            Result->Hptr = NewPtr;
            
            memcpy(Result->Hptr + Result->Len, GetStr(Left), Left->Len);
            memcpy(Result->Hptr + Result->Len + Left->Len, Right, RightLen);
            Result->Hptr[NewSize] = 0;
        }
        else
        {
            memcpy(Result->Sptr + Result->Len, Left->Sptr, Left->Len);
            memcpy(Result->Sptr + Result->Len + Left->Len, Right, RightLen);
        }
        
        Result->Len = NewSize;
    }
}

void Tstring(tstring *Str, tagged_heap_block *TaggedHeap, const char *Cstr, u32 Len)
{
    
}

void TstringAdd(tstring *Result, tagged_heap_block *TaggedHeap, const tstring *Left, const tstring *Right)
{
    
}

void StringAdd(mstring *Result, const char *Left, u32 LeftLen, const char *Right, u32 RightLen)
{
    u32 Size = Result->Len + LeftLen + RightLen;
    
    if (Result->Heap)
    {
        if (Size > Result->ReservedHeapSize)
        {
            Result->ReservedHeapSize = (Result->ReservedHeapSize * 2 > Size) ? Result->ReservedHeapSize*2 : Size + 1;
            
            char *NewPtr = palloc<char>(&StringArena, Result->ReservedHeapSize);
            memcpy(NewPtr, Result->Hptr, Result->Len);
            pfree<char>(&StringArena, Result->Hptr);
            Result->Hptr = NewPtr;
        }
        
        memcpy(Result->Hptr + Result->Len, Left, LeftLen);
        memcpy(Result->Hptr + Result->Len + LeftLen, Right, RightLen);
        Result->Hptr[Size] = 0;
    }
    else if (Size > STRING_STACK_SIZE - 1)
    {
        Result->Heap = 1;
        Result->ReservedHeapSize = Size + 1;
        
        char *NewPtr = palloc<char>(&StringArena, Result->ReservedHeapSize);
        memcpy(NewPtr, Result->Sptr, Result->Len);
        Result->Hptr = NewPtr;
        
        memcpy(Result->Hptr + Result->Len, Left, LeftLen);
        memcpy(Result->Hptr + Result->Len + LeftLen, Right, RightLen);
        Result->Hptr[Size] = 0;
    }
    else
    {
        memcpy(Result->Sptr + Result->Len, Left, LeftLen);
        memcpy(Result->Sptr + Result->Len + LeftLen, Right, RightLen);
        Result->Sptr[Size] = 0;
    }
    
    Result->Len = Size;
}

void TaggedStringAdd(tstring *Result, tagged_heap_block *TaggedHeap, const char *Left, u32 LeftLen, const char *Right, u32 RightLen)
{
    
}

// String comparison that is valid for both mstring and tstring
bool StringCmp(const char *Left, u32 LeftLen, const char *Right, u32 RightLen)
{
    return false;
}

#endif
