#ifndef PLATFORM_ENTITY_MANAGER_H
#define PLATFORM_ENTITY_MANAGER_H

/*

Needs to be free list allocator (to keep it simple)

1. Entities do not have fixed size
2. Components do not have fixed size and are unknown

o

*/

typedef struct entity_manager
{
    
    
    
} entity_manager;


typedef struct entity
{
    u64 Offset:32; // Offset into memory structure (fast freeing?)
    u64 Index:24;  // Index into....where?
    u64 Gen:8;     // Generation for this entity...but why?
} entity;

#endif //PLATFORM_ENTITY_MANAGER_H
