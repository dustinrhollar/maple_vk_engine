#ifndef JENGINE_ECS_ENTITY_H
#define JENGINE_ECS_ENTITY_H

/*

An Entity is a wrapper around a uint64_t type where the first 48 bits
represent an index and the last 16 bits represent a generation. This means
that ther can be a total of 2^48 entites and each entity can have 2^16
generations before wrapping back to 0. However, there probably
won't ever actually be 2^48 entites in the system, so a MAX_ENTITY global
is defined in the ecs namespace. It is expected that this value represent
that total number of entities that could ever be alive the system. This number
can be determined by profiling the system.

In order to manage entities, two lists within are used:
1. EntityRegistry
2. FreeIndices

The EntityRegistry contains all possible entities with the system and is used
to verify the validity of an entity. The index bits of an Entity is the index
into EntityRegistry and the generation bits are what are stored within the list. An entity
"generation" is what signifies an entity's uniqueness. When an entity is deleted,
it is possible for the index the Entity was stored at to be reused - so a generational
number is used to signify what "generation" that index is at. If two entities happen
to share the same index, then their generations can be checked against the EntityRegistry
to determine which of the two (or both) are no longer valid.

The free indices list maintains a list of indices of Entities that have been destroyed
and are ready to be reused. However, these this list is not immediately queried upon
entity creation. This is done to prevent the same index being used over and over in a
case where entities are being destroyed and created at a fast rate. A global called
MIN_FREE_INDICES is used to determine when the free indice list can be queried to reuse
indices. This list acts as a queue (doubly linked list), so adding and removing should
occur in constant time.

User API

struct Entity
- index()
- generation()

void IntializeEntityRegistry();
- Initializes the entity registry

void ShutdownEntityRegistry();
- Shuts the entity registry down

Entity CreateEntity();
- Creates a new entitiy

bool IsValidEntity(Entity entity);
- Determines if an entity is valid by comparing the parameters generation
  to the generation number in the Registry. Returns true if valid, false
  otherwise.

void DestroyEntity(Entity entity);
- Kills an entity by updating the registry's generation number. All components
  are removed from the entity.

void AttachComonentToEntity(Entity entity, GUID component_id);
- Attached the component with the specified ID. It is advised  to call
  AddEntityToComponent<T> instead through the Component interface.

void DetachComponentFromEntity(Entity entity, GUID component_id);
- Detaches the component with the specified ID. It is advised  to call
  RemoveEntityFromComponent<T> instead through the Component interface.

LinkedList<GUID> *GetAttachedComponents(Entity entitiy);
- Gets the attached components from an entitiy.

*/

namespace ecs {
    
    // max allowed entities in a scene
    static const u32 MAX_ENTITIES = 128;
    
    static const u64 ENTITY_INDEX_BITS = 48;
    static const u64 ENTITY_INDEX_MASK = (((u64)1)<<ENTITY_INDEX_BITS)-1;
    
    static const u64 ENTITY_GENERATION_BITS = 16;
    static const u64 ENTITY_GENERATION_MASK = (((u64)1)<<ENTITY_GENERATION_BITS)-1;
    
    struct Entity
    {
        // Top 48bits is the index
        // Lower 16bits is the generation
        EID id;
        
        u64 index() const {return id & ENTITY_INDEX_MASK;}
        u64 generation() const {return (id >> ENTITY_INDEX_BITS) & ENTITY_GENERATION_MASK;}
    };
    
    void IntializeEntityRegistry();
    void ShutdownEntityRegistry();
    
    void LoadEntityRegistryFromFile(jstring &entity_registry);
    void FlushEntityRegistryToFile(jstring &entity_registry);
    
    Entity CreateEntity();
    bool IsValidEntity(Entity entity);
    void DestroyEntity(Entity entity);
    
    void AttachComonentToEntity(Entity entity, EID component_id);
    void DetachComponentFromEntity(Entity entity, EID component_id);
    
    DynamicArray<EID> *GetAttachedComponents(Entity entitiy);
    bool IsComponentAttached(Entity entity, EID component_id);
} // ecs

#endif // JENGINE_ECS_ENTITY_H
