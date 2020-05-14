#ifndef ENGINE_UTILS_JSTRING_H
#define ENGINE_UTILS_JSTRING_H

/*

jstring is a header-only string library that works as an alternative to
std::string. All strings are null-terminated.

jstring has 2 ways of storing the string: stack and heap allocated storage.
Small strings less than 12 bytes will be stored in the stack allocated storage.
In order to reduce the overall size of jstring, heap storage and static storage
are within a union. The size of the jstring is 16 bytes,

For heap storage string, they are originally allocated to be size + 1. Any
further resizes double the heap size.

It should be noted that a user should be careful when invoking the
equals operator using malloc or other custom allocation schemes. The new operator
invokes the default constructor (unless otherwise specified), however alternate
allocation schemes will not invoke a constructor. This is important in the following
case:

jstring *str = (jstring*)malloc(sizeof(jstring));
str = "test";

In this example, nothing neems amiss. "str" is allocated and then the operator= is
 invoked. However, if a user then does:

char *cstr = "other string";
str = cstr;

Again, the operator= is called to move "cstr" into "str". However, this will result
in a memory leak. This implementation of jstring will not free up the heap when
operator= is called. The reason for this is because if a heap allocation occurs
outside new, then the memory is not initialized, so if the implementation of the
operator= attempts to free any allocated memory, it will be impossible to tell if
the heap memory is actually being.

*/

// NOTE(Dustin): Careful with this implementation...
// len and heap are only in the first struct, but it
// should be fine. They are accessible whether or not
// heap is in use.
union jstring {
    struct {
        char sptr[12];
        unsigned len:31;
        unsigned heap:1; // 0 - heap is not used. 1 - heap is used
    };
    
    struct {
        char *hptr;
        unsigned reserved_heap_size;
    };
    
    inline void Clear();
    char *GetCStr() const;
    
    inline jstring& operator+=(const jstring &str);
    inline jstring& operator+=(const char *cstr);
    inline jstring& operator+=(char ch);
    
    inline char& operator[](int idx);
    bool operator==(const jstring &rhs) const;
    bool operator==(const char *rhs) const;
};

//~ Initializers

jstring InitJString();
jstring InitJString(const char *str);
jstring InitJString(const char *str, unsigned size);
jstring InitJString(unsigned req_heap_size);

//~
// USE WITH CARE - Copy on Write implementation
inline jstring operator+(const jstring &lhs, const jstring &str);
inline jstring operator+(const jstring &lhs, const char *cstr);
inline jstring operator+(const char *cstr, const jstring &rhs);
inline jstring operator+(const jstring &lhs, char ch);
inline jstring operator+(char ch, const jstring& rhs);

//~
// Preferred usage over jstring operator+
// rather than using COW, it takes in the reference of the returned
// value so a copy/move is not invoked upon return. If result already
// contains a string, the lhs and rhs is appended to that string. In other
// words, the original string is not overwritten.
void AddJString(jstring& result, const jstring &lhs, const jstring &rhs);
void AddJString(jstring &result, const char* lhs,    const jstring &rhs);
void AddJString(jstring &result, const jstring &lhs, const char* rhs);
void AddJString(jstring &result, const char* lhs,    const char* rhs);


//~ Do some intersting processing on a jstring

inline void ToLowerCase(jstring &str);
inline i32 LastIndexOf(jstring &str, char ch);

//~ Copy a jstring.
jstring CopyJString(jstring Src);

//~
// Useful macros for creating jstrings
//------------------------------------------------
// NOTE(Dustin): Currently tstring implementation is identical to pstring
// I am keeping this function call for 2 reasons:
// 1. Backwards compatibility. tstring is used in a lot of places.
// 2. I might add back in temporary storage strings, so i would like to keep
//    that function call around just in case.
inline jstring pstring() {return InitJString();}
inline jstring pstring(const char *p) {return InitJString(p);}
inline jstring pstring(const char *p, unsigned s) {return InitJString(p, s);}

inline jstring tstring() {return InitJString();}
inline jstring tstring(const char *p) {return InitJString(p);}
inline jstring tstring(const char *p, unsigned s) {return InitJString(p, s);}

template<class jstring>
u128 Hash(const jstring &str)
{
    u128 result = {};
    Hash128(str.GetCStr(), str.len, &result);
    return result;
}

#endif //JSTRING_H

//~
#if defined(MAPLE_JSTRING_IMPLEMENTATION)

#if !defined(pstring_alloc) || !defined(pstring_realloc) || !defined(pstring_free)

#define pstring_alloc   malloc
#define pstring_realloc realloc
#define pstring_free    free

#endif

#if !defined(tstring_alloc) || !defined(tstring_realloc) || !defined(tstring_free)

#define tstring_alloc   malloc
#define tstring_realloc realloc
#define tstring_free    free

#endif

jstring InitJString()
{
    jstring result;
    
    result.len = 0;
    result.heap = 0;
    
    return result;
}

jstring InitJString(const char *str)
{
    jstring result = {};
    
    result.len = strlen(str);
    result.heap = 0;
    
    if (result.len > 11)
    {
        result.heap = 1;
        result.reserved_heap_size = result.len + 1;
        result.hptr = (char*)pstring_alloc(result.reserved_heap_size);
        
        memcpy(result.hptr, str, result.len);
        result.hptr[result.len] = 0;
    }
    else
    {
        memcpy(result.sptr, str, result.len);
        result.sptr[result.len] = 0;
    }
    
    return result;
}

jstring InitJString(const char *str, unsigned size)
{
    jstring result = {};
    
    result.len = size;
    result.heap = 0;
    
    if (result.len > 11)
    {
        result.heap = 1;
        result.reserved_heap_size = result.len + 1;
        result.hptr = (char*)pstring_alloc(result.reserved_heap_size);
        
        memcpy(result.hptr, str, size);
        result.hptr[result.len] = 0;
    }
    else
    {
        memcpy(result.sptr, str, size);
        result.sptr[result.len] = 0;
    }
    
    return result;
}

// if legnth is greater than 11, then space
// is reserved on the heap length+1
jstring InitJString(unsigned req_heap_size)
{
    jstring result = {};
    
    result.len = 0;
    result.heap = 0;
    
    if (req_heap_size > 11) {
        result.heap = 1;
        result.reserved_heap_size = req_heap_size + 1;
        result.hptr = (char*)pstring_alloc(result.reserved_heap_size);
    }
    
    return result;
}

inline void jstring::Clear()
{
    if (heap)
    {
        pstring_free(hptr);
        hptr = nullptr;
        reserved_heap_size = 0;
    }
    
    len = 0;
    heap = 0;
}

inline jstring& jstring::operator+=(const jstring &str)
{
    /*
Conditions:

1. Both *this and str are heap alloc'd
 2. *this is heap alloc'd
3. str is heap alloc'd
4. Neither is heap alloc'd, but their sum need to be alloc'd
5. Neither is heap alloc'd, and their sum is less than 11

Confidently say:
- If at least one of them has a length greater 11, the result is heap alloc'd
- If their sum is less than 11, then neither are heap alloc'd

*/
    unsigned lhs_sz = len;
    unsigned rhs_sz = str.len;
    
    len += rhs_sz;
    
    if (heap)
    {
        if (len > reserved_heap_size)
        {
            reserved_heap_size = (reserved_heap_size * 2 > len) ? reserved_heap_size * 2 : len + 1;
            hptr = (char*)pstring_realloc(hptr, reserved_heap_size);
        }
        
        memcpy(hptr + lhs_sz, (str.heap) ? str.hptr : str.sptr, rhs_sz);
        hptr[len] = 0;
    }
    else if (len > 11)
    {
        heap = 1;
        reserved_heap_size = len + 1;
        hptr = (char*)pstring_alloc(reserved_heap_size);
        
        memcpy(hptr, sptr, lhs_sz);
        memcpy(hptr + lhs_sz, (str.heap) ? str.hptr : str.sptr, rhs_sz);
        hptr[len] = 0;
    }
    else
    {
        memcpy(hptr + lhs_sz, str.sptr, rhs_sz);
        sptr[len] = 0;
    }
    
    return *this;
}

// Null Terminated string
inline jstring& jstring::operator+=(const char *cstr)
{
    /*
Conditions:

1. *this is heap alloc'd
 4. *this is not heap alloc'd, but their sum needs to be alloc'd
5. *this is not heap alloc'd, and their sum is less than 11

Confidently say:
- If at least one of them has a length greater 11, the result is heap alloc'd
- If their sum is less than 11, then neither are heap alloc'd

*/
    unsigned lhs_sz = len;
    unsigned rhs_sz = (unsigned)strlen(cstr);
    
    len += rhs_sz;
    
    if (heap)
    {
        if (len > reserved_heap_size)
        {
            reserved_heap_size = (reserved_heap_size * 2 > len) ? reserved_heap_size * 2 : len + 1;
            hptr = (char*)pstring_realloc(hptr, reserved_heap_size);
        }
        
        memcpy(hptr + lhs_sz, cstr, rhs_sz);
    }
    else if (len > 11)
    {
        heap = 1;
        reserved_heap_size = len + 1;
        hptr = (char*)pstring_alloc(reserved_heap_size);
        
        memcpy(hptr, sptr, lhs_sz);
        memcpy(hptr + lhs_sz, cstr, rhs_sz);
    }
    else
    {
        memcpy(hptr + lhs_sz, cstr, rhs_sz);
    }
    
    return *this;
}

inline jstring& jstring::operator+=(char ch)
{
    len++;
    
    if (heap)
    {
        if (len > reserved_heap_size)
        {
            reserved_heap_size = (reserved_heap_size * 2 > len) ? reserved_heap_size * 2 : len + 1;
            hptr = (char*)pstring_realloc(hptr, reserved_heap_size);
        }
        
        hptr[len-1] = ch;
        hptr[len]   = 0;
    }
    else if (len > 11)
    {
        heap = 1;
        reserved_heap_size = len + 1;
        hptr = (char*)pstring_alloc(reserved_heap_size);
        
        hptr[len-1] = ch;
        hptr[len]   = 0;
    }
    else
    {
        sptr[len-1] = ch;
        sptr[len]   = 0;
    }
    
    return *this;
}


//-----------------------------------------
// USE WITH CARE - Copy on Write implementation
inline jstring operator+(const jstring &lhs, const jstring &rhs)
{
    jstring result = InitJString(lhs.len + rhs.len);
    result.len = lhs.len + rhs.len;
    
    if (result.heap)
    {
        memcpy(result.hptr, (lhs.heap) ? lhs.hptr : lhs.sptr, lhs.len);
        memcpy(result.hptr + lhs.len, (rhs.heap) ? rhs.hptr : rhs.sptr, rhs.len);
        result.hptr[result.len] = 0;
    }
    else
    {
        memcpy(result.hptr, lhs.sptr, lhs.len);
        memcpy(result.hptr + lhs.len, rhs.sptr, rhs.len);
        result.sptr[result.len] = 0;
    }
    
    return result;
}

inline jstring operator+(const jstring &lhs, const char *cstr)
{
    unsigned str_len = (unsigned)strlen(cstr);
    
    jstring result = InitJString(lhs.len + str_len);
    result.len = lhs.len + str_len;
    
    if (result.heap)
    {
        memcpy(result.hptr, (lhs.heap) ? lhs.hptr : lhs.sptr, lhs.len);
        memcpy(result.hptr + lhs.len, cstr , str_len);
        result.hptr[result.len] = 0;
    }
    else
    {
        memcpy(result.hptr, lhs.sptr, lhs.len);
        memcpy(result.hptr + lhs.len, cstr , str_len);
        result.sptr[result.len] = 0;
    }
    
    return result;
}

inline jstring operator+(const char *cstr, const jstring &rhs)
{
    unsigned str_len = (unsigned)strlen(cstr);
    
    jstring result = InitJString(rhs.len + str_len);
    result.len = rhs.len + str_len;
    
    if (result.heap)
    {
        memcpy(result.hptr, cstr , str_len);
        memcpy(result.hptr + str_len, (rhs.heap) ? rhs.hptr : rhs.sptr, rhs.len);
        result.hptr[result.len] = 0;
    }
    else
    {
        memcpy(result.hptr, cstr , str_len);
        memcpy(result.hptr + str_len, rhs.sptr, rhs.len);
        result.sptr[result.len] = 0;
    }
    
    return result;
}

inline jstring operator+(const jstring& lhs, char ch)
{
    jstring result = InitJString(lhs.len + 1);
    result.len = lhs.len + 1;
    
    if (result.heap)
    {
        memcpy(result.hptr, (lhs.heap) ? lhs.hptr : lhs.sptr, lhs.len);
        result.hptr[result.len-1] = ch;
        result.hptr[result.len]   = 0;
    }
    else
    {
        memcpy(result.sptr, lhs.sptr, lhs.len);
        result.sptr[result.len-1] = ch;
        result.sptr[result.len]   = 0;
    }
    
    return result;
}

inline jstring operator+(char ch, const jstring& rhs)
{
    jstring result = InitJString(rhs.len + 1);
    result.len = rhs.len + 1;
    
    if (result.heap)
    {
        result.hptr[0] = ch;
        memcpy(result.hptr + 1, (rhs.heap) ? rhs.hptr : rhs.sptr, rhs.len);
        result.hptr[result.len] = 0;
    }
    else
    {
        result.sptr[0] = ch;
        memcpy(result.sptr + 1, rhs.sptr, rhs.len);
        result.sptr[result.len] = 0;
    }
    
    return result;
}

//-----------------------------------------

void AddJString(jstring &result, const jstring &lhs, const jstring &rhs)
{
    unsigned add_len = result.len + lhs.len + rhs.len;
    
    if (result.heap)
    {
        if (result.reserved_heap_size <= add_len)
        {
            result.reserved_heap_size = (result.reserved_heap_size * 2 > add_len) ? result.reserved_heap_size * 2 : add_len + 1;
            result.hptr = (char*)pstring_realloc(result.hptr, result.len);
        }
        
        memcpy(result.hptr + result.len, (lhs.heap) ? lhs.hptr : lhs.sptr, lhs.len);
        memcpy(result.hptr + result.len + lhs.len, (rhs.heap) ? rhs.hptr : rhs.sptr, rhs.len);
        result.hptr[add_len] = 0;
    }
    else if (add_len > 11)
    {
        result.heap = 1;
        result.reserved_heap_size = add_len + 1;
        result.hptr = (char*)pstring_alloc(result.reserved_heap_size);
        
        memcpy(result.hptr, result.sptr, result.len);
        memcpy(result.hptr + result.len, (lhs.heap) ? lhs.hptr : lhs.sptr, lhs.len);
        memcpy(result.hptr + result.len + lhs.len, (rhs.heap) ? rhs.hptr : rhs.sptr, rhs.len);
        result.hptr[add_len] = 0;
    }
    else
    {
        memcpy(result.sptr + result.len, lhs.sptr, lhs.len);
        memcpy(result.sptr + result.len + lhs.len, rhs.sptr, rhs.len);
        result.sptr[add_len] = 0;
    }
    
    result.len = add_len;
}

void AddJString(jstring &result, const char* lhs, const jstring &rhs)
{
    unsigned str_len = (unsigned)strlen(lhs);
    unsigned add_len = result.len + str_len + rhs.len;
    
    if (result.heap)
    {
        if (result.reserved_heap_size <= add_len)
        {
            result.reserved_heap_size = (result.reserved_heap_size * 2 > add_len) ? result.reserved_heap_size * 2 : add_len + 1;
            result.hptr = (char*)pstring_realloc(result.hptr, result.len);
        }
        
        memcpy(result.hptr + result.len, lhs, str_len);
        memcpy(result.hptr + result.len + str_len, (rhs.heap) ? rhs.hptr : rhs.sptr, rhs.len);
        result.hptr[add_len] = 0;
    }
    else if (add_len > 11)
    {
        result.heap = 1;
        result.reserved_heap_size = add_len + 1;
        result.hptr = (char*)pstring_alloc(result.reserved_heap_size);
        
        memcpy(result.hptr, result.sptr, result.len);
        memcpy(result.hptr + result.len, lhs, str_len);
        memcpy(result.hptr + result.len + str_len, (rhs.heap) ? rhs.hptr : rhs.sptr, rhs.len);
        result.hptr[add_len] = 0;
    }
    else
    {
        memcpy(result.sptr + result.len, lhs, str_len);
        memcpy(result.sptr + result.len + str_len, rhs.sptr, rhs.len);
        result.sptr[add_len] = 0;
    }
    
    result.len = add_len;
}

void AddJString(jstring &result, const jstring &lhs, const char* rhs)
{
    unsigned str_len = (unsigned)strlen(rhs);
    unsigned add_len = result.len + str_len + lhs.len;
    
    if (result.heap)
    {
        if (result.reserved_heap_size <= add_len)
        {
            result.reserved_heap_size = (result.reserved_heap_size * 2 > add_len) ? result.reserved_heap_size * 2 : add_len + 1;
            result.hptr = (char*)pstring_realloc(result.hptr, result.len);
        }
        
        memcpy(result.hptr + result.len, (lhs.heap) ? lhs.hptr : lhs.sptr, lhs.len);
        memcpy(result.hptr + result.len + lhs.len, rhs, str_len);
        result.hptr[add_len] = 0;
    }
    else if (add_len > 11)
    {
        result.heap = 1;
        result.reserved_heap_size = add_len + 1;
        result.hptr = (char*)pstring_alloc(result.reserved_heap_size);
        
        memcpy(result.hptr, result.sptr, result.len);
        memcpy(result.hptr + result.len, (lhs.heap) ? lhs.hptr : lhs.sptr, lhs.len);
        memcpy(result.hptr + result.len + lhs.len, rhs, str_len);
        result.hptr[add_len] = 0;
    }
    else
    {
        memcpy(result.sptr + result.len, lhs.sptr, lhs.len);
        memcpy(result.sptr + result.len + lhs.len, rhs, str_len);
        result.sptr[add_len] = 0;
    }
    
    result.len = add_len;
}

void AddJString(jstring &result, const char* lhs, const char* rhs)
{
    unsigned lhs_sz = (unsigned)strlen(lhs);
    unsigned rhs_sz = (unsigned)strlen(rhs);
    unsigned add_len = result.len + lhs_sz + rhs_sz;
    
    if (result.heap)
    {
        if (result.reserved_heap_size <= add_len)
        {
            result.reserved_heap_size = (result.reserved_heap_size * 2 > add_len) ? result.reserved_heap_size * 2 : add_len + 1;
            result.hptr = (char*)pstring_realloc(result.hptr, result.len);
        }
        
        memcpy(result.hptr + result.len, lhs, lhs_sz);
        memcpy(result.hptr + result.len + lhs_sz, rhs, rhs_sz);
        result.hptr[add_len] = 0;
    }
    else if (add_len > 11)
    {
        result.heap = 1;
        result.reserved_heap_size = add_len + 1;
        result.hptr = (char*)pstring_alloc(result.reserved_heap_size);
        
        memcpy(result.hptr, result.sptr, result.len);
        memcpy(result.hptr + result.len, lhs, lhs_sz);
        memcpy(result.hptr + result.len + lhs_sz, rhs, rhs_sz);
        result.hptr[add_len] = 0;
    }
    else
    {
        memcpy(result.sptr + result.len, lhs, lhs_sz);
        memcpy(result.sptr + result.len + lhs_sz, rhs, rhs_sz);
        result.sptr[add_len] = 0;
    }
}

char* jstring::GetCStr() const
{
    // NOTE(Dustin): Redundant if statement?
    if (heap) return hptr;
    else      return (char*)sptr;
}

inline char& jstring::operator[](int idx)
{
    return (heap) ? hptr[idx] : sptr[idx];
}

bool jstring::operator==(const jstring &rhs) const
{
    u128 lhs_hash = {};
    u128 rhs_hash = {};
    
    Hash128((heap) ? hptr : sptr, len, &lhs_hash);
    Hash128((rhs.heap) ? rhs.hptr : rhs.sptr, rhs.len, &rhs_hash);
    
    return (lhs_hash.upper == rhs_hash.upper) && (lhs_hash.lower == rhs_hash.lower);
}

bool jstring::operator==(const char* rhs) const
{
    u128 lhs_hash = {};
    u128 rhs_hash = {};
    
    Hash128((heap) ? hptr : sptr, len, &lhs_hash);
    Hash128(rhs, strlen(rhs), &rhs_hash);
    
    return (lhs_hash.upper == rhs_hash.upper) && (lhs_hash.lower == rhs_hash.lower);
}

inline void ToLowerCase(jstring &str)
{
    char *iter = nullptr;
    if (str.heap)
    {
        iter = str.hptr;
    }
    else
    {
        iter = str.sptr;
    }
    
    for (u32 i = 0; i < str.len; ++i)
    {
        if (iter[i] > 64 && iter[i] < 91)
        {
            iter[i] += 32;
        }
    }
}

inline i32 LastIndexOf(jstring &str, char ch)
{
    char *iter = nullptr;
    
    if (str.heap)
    {
        iter = str.hptr;
    }
    else
    {
        iter = str.sptr;
    }
    
    i32 idx = -1;
    for (i32 i = 0; i < str.len; ++i)
    {
        if (iter[i] == ch)
        {
            idx = i;
        }
    }
    
    return idx;
}

jstring CopyJString(jstring Src)
{
    jstring Result = InitJString(Src.GetCStr());
    return Result;
}


#endif