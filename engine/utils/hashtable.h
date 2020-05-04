#ifndef ENGINE_UTILS_HASHTABLE_H
#define ENGINE_UTILS_HASHTABLE_H

#if defined(MAPLE_HASHTABLE_IMPLEMENTATION)

// TODO(Dustin): Allow for the table to resize

#include <stdint.h>

// NOTE(Dustin): I don't think this is a thing anymore
void InitializeHash();

// Fixed size hashtable
template<class K, class V>
class HashTable {
    public:
    HashTable();
    HashTable(unsigned capacity);
    ~HashTable();
    
    // Copy/Move Constructors/Operators are not allowed
    HashTable(HashTable &other);
    HashTable(HashTable &&other);
    
    HashTable<K,V>& operator=(HashTable &cpy);
    HashTable<K,V>& operator=(HashTable &&cpy);
    
    void Reset();
    
    bool Insert(const K& key, const V& value);
    V*   Get(const K &key);
    void Delete(const K& key);
    
    struct Entry {
        bool IsEmpty;
        u128 HashedKey;
        V    Value;
    };
    
    Entry *GetEntries() {return Entries;}
    
    unsigned Count;
    unsigned Capacity;
    
    Entry *Entries;
    
    private:
    
    static constexpr float LoadFactor = 0.60f;
    
    Entry *GetEntry(const K& key);
};

#if !defined(ptable_alloc) || !defined(ptable_realloc) || !defined(ptable_free)

#define ptable_alloc   malloc
#define ptable_realloc realloc
#define ptable_free    free

#endif

#if !defined(ttable_alloc) || !defined(ttable_realloc) || !defined(ttable_free)

#define ttable_alloc   malloc
#define ttable_realloc realloc
#define ttable_free    free

#endif

template<class K, class V>
HashTable<K,V>::HashTable()
: Count(0)
, Entries(nullptr)
, Capacity(0)
{
}

template<class K, class V>
HashTable<K,V>::HashTable(unsigned capacity)
: Count(0)
{
    Capacity = (unsigned)(capacity + (capacity * LoadFactor));
    
    Entries = (Entry*)ptable_alloc(sizeof(Entry) *Capacity);
    for (unsigned i = 0; i < Capacity; ++i) {
        Entries[i] = {};
        Entries[i].IsEmpty = true;
    }
}

template<class K, class V>
HashTable<K,V>::HashTable(HashTable &other)
: Count(other.count)
, Entries(nullptr)
, Capacity(other.Capacity)
{
    Entries = (Entry*)ptable_alloc(sizeof(Entry) *Capacity);
    for (unsigned i = 0; i < Capacity; ++i) {
        Entries[i] = other.Entries[i];
    }
}

template<class K, class V>
HashTable<K,V>::HashTable(HashTable &&other)
: Count(other.count)
, Entries(nullptr)
, Capacity(other.Capacity)
{
    Entries = (Entry*)ptable_alloc(sizeof(Entry) *Capacity);
    for (unsigned i = 0; i < Capacity; ++i) {
        Entries[i] = other.Entries[i];
    }
    
    other.Reset();
}

template<class K, class V>
HashTable<K,V>& HashTable<K,V>::operator=(HashTable &cpy) {
    Count    = cpy.Count;
    Capacity = cpy.Capacity;
    
    Entries = (Entry*)ptable_alloc(sizeof(Entry) *Capacity);
    for (unsigned i = 0; i < Capacity; ++i) {
        Entries[i] = cpy.Entries[i];
    }
    
    return *this;
}

template<class K, class V>
HashTable<K,V>& HashTable<K,V>::operator=(HashTable &&cpy) {
    Count    = cpy.Count;
    Capacity = cpy.Capacity;
    
    Entries = (Entry*)ptable_alloc(sizeof(Entry) *Capacity);
    for (unsigned i = 0; i < Capacity; ++i) {
        Entries[i] = cpy.Entries[i];
    }
    
    cpy.Reset();
    
    return *this;
}


template<class K, class V>
HashTable<K,V>::~HashTable()
{
    Reset();
}

template<class K, class V>
void HashTable<K,V>::Reset()
{
    if (Entries)
        ptable_free(Entries);
    
    Entries = nullptr;
    Capacity = 0;
    Count = 0;
}

template<class K, class V>
typename HashTable<K,V>::Entry* HashTable<K,V>::GetEntry(const K& key) {
    u128 hashed_key = Hash<K>(key);
    
    // use lower 64 bits as the index into the entry array
    u64 index = (abs(hashed_key.lower)) % Capacity;
    Entry *entry = nullptr;
    for (;;) {
        entry = &Entries[index];
        
        if (entry->IsEmpty || // index is not occupies
            (entry->HashedKey.upper == hashed_key.upper && // index is occupied, but
             entry->HashedKey.lower == hashed_key.lower))  // elements contain same hash
        {
            return entry;
        }
        
        index = (index + 1) % Capacity;
    }
}


template<class K, class V>
bool HashTable<K,V>::Insert(const K& key, const V& value) {
    Entry *entry = GetEntry(key);
    
    if (entry->IsEmpty)
    {
        Count++;
        
        u128 hashed_key = Hash<K>(key);
        
        entry->HashedKey = hashed_key;
        entry->Value     = value;
        entry->IsEmpty   = false;
        
        return true;
    }
    else
    {
        return false;
    }
    
}

template<class K, class V>
V* HashTable<K,V>::Get(const K &key) {
    if (Count == 0) return false;
    
    Entry *entry = GetEntry(key);
    
    if (!entry->IsEmpty)
    {
        return &entry->Value;
    }
    else return nullptr;
}

template<class K, class V>
void HashTable<K,V>::Delete(const K& key) {
    if (Count == 0) return;
    
    Entry *entry = GetEntry(key);
    entry->IsEmpty = true;
    Count--;
}

#endif // #define JENGINE_UTILS_HASHTABLE_IMPLEMENTATION

#endif