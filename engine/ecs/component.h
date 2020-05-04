#ifndef JENGINE_ECS_COMPONENT_H
#define JENGINE_ECS_COMPONENT_H

/*

The Component portion of the ECS acts mainly as a
memory manager for component memory. Internally, component.cpp
keeps track of two lists:
1. Component Registry
2. Component Cache

The Component Registry is a look up table that takes a component
id and an entity and can find the associated component data.
This table is a resizable list where each component allocated a
block of memory of size (sizeof(T) * MAX_ELEMENTS), where T is the
registered component type.

Component data is stored based on the index of the Entity, which means
that as entities are removed/added, it is likely that component data
storage is sparse and non-contiguous. In order to improve iteration time
over component data, a Component Cache is used. Each Component Cache (one
per component type) maintains a list of size (MAX_ENTITIES) and stores
Entities in contiguous order that are attached to the component. When
removing entities from a Component Cache, the last entity in the cache
is swapped with the Entity being removed. Due to this swapping, it is not
guarenteed that that entities are in contiguous order like it is for
Component Data.

As entities are destroyed, their corresponding components are marked as
inactive. In order to group a variety of component types that are of relatively
unknown tyoes to the ECS system, all components MUST inherit from the IComponent
base class. This class is a wrapper around a single boolean that is used to mark
components as inactive for an Entity. Failing to inherit from IComponet will
result in a compile-time error.

The last important structure of Component is the ComponentIterator<T>. It is a
simple structure, containing then next index and function called next(). When
next() is called on the ComponentIterator, the iterator used the corresponding
Component Cache to find the next component data to return to the caller.

There are 3 ways to access component data:
1. Raw component data through "T* GetComponentData<T>()"
2. Component Iterator with no inplace swapping using "T* ComponentIter<T>::next(false)"
---- Requires the function "void FlushComponentCache<T>()"
3. Component Iterator with inplace swapping "T* ComponentIter<T>::next(true)"

Getting the Component Data directly will return the non-contiguous storage as
a raw pointer. When iterating over this list, it is important to check if a
component is active for that entity, otherwise you will be potentially
performing transformations on a dead entity.

Using the ComponentIterator without swapping allows for fast contiguous iteration
over component data without having to check if entities are active. The downside
to not using the swapping method means that it is not guarenteed that all entities
in the cache are still active, so some cycles will be wasted processing inactive
entities. In order to flush the cache, "void FlushComponentCache()" can be used
which performs the swapping mechanism, but traverses the entire cache to remove
all unused elements. Time complexity for flushing the cache will always be O(n),
where n is the size of the cache. This will result in having to traverse the cache
twice: once for the transformations and a second time for flushing, but for sparse
component data, it will still be cheaper than traversing the all component data.

Component Iterator with swapping is the default approach with the iterator. If the
iterator determiens that the next element is inactive, it will go ahead and swap it
out of the cache. This will preserve O(1) removal and traversal removing the need
to flush the cache when elements are removed.

Some notes on performance for each approach to iterating over component data. The
first method is the best approach if the desired functionality is to always access
sequential memory, at the cost of having to process inactive entities. However,
worst case for this method will always be O(N), where N is the total number of entities
in the system. The second approach removes the problem of having to process inactive
entities, but memory access is no longer guarenteed to be contiguous. However, component
data is stored in contiguous memory, so access will be in the same memory block. The
major cost of this approach is having to Flush() the cache at specified intervals.
The final approach removes the need to flush the cache at the cost of extra instructions.
This cost can be seen when the component data storage is near full and the iterator
is having to do unnecessary pointer indirection. However, for sparse storages, this
final method is faster than the previous two.

User API:

IComponent Interface
struct ComponentIter as an iterator


void InitializeComponentRegistry();
void ShutdownComponentRegistry();

GUID RegisterComponent<T>();

GUID GetComponentId<T>();

T* GetComponentData<T>();

ComponentIter<T> GetComponentIter<T>();
--- next()

void FlushComponentCache<T>();

*/

namespace ecs {
    
    // Each component gets a unique id
    extern EID STATIC_COMPONENT_GUID;
    // Generate an id for a new type of component
    template<class T>
        EID GenerateNewComponentID()
    {
        return STATIC_COMPONENT_GUID++;
    }
    
    // Basic interface that all components should inherit from.
    // It is a wrapper around a boolean that determines if a component
    // is active.
    class IComponent
    {
        public:
        bool IsActive;
    };
    
    // A static class that acts as a wrapper around a static id for component types.
    template<class T>
        class Component
    {
        public:
        EID GetStaticId() {return STATIC_COMPONENT_ID;}
        static const EID STATIC_COMPONENT_ID;
    };
    
    template<class T>
        const EID Component<T>::STATIC_COMPONENT_ID = GenerateNewComponentID<T>();
    
    // Initialize and shutdown the ComponentRegistry
    void InitializeComponentRegistry();
    void ShutdownComponentRegistry();
    
    void AddEntityToComponent(EID component_id, Entity entity, void *data, size_t size_of_component);
    template<class T>
        void AddEntityToComponent(Entity entity, T *data)
    {
        AddEntityToComponent(Component<T>::STATIC_COMPONENT_ID, entity, (void*)data, sizeof(T));
        AttachComonentToEntity(entity, Component<T>::STATIC_COMPONENT_ID);
    }
    
    void RemoveEntityFromComponent(EID component_id, Entity entity);
    template<class T>
        void RemoveEntityFromComponent(Entity entity)
    {
        RemoveEntityFromComponent(Component<T>::STATIC_COMPONENT_ID, entity);
        DetachComponentFromEntity(entity, Component<T>::STATIC_COMPONENT_ID);
    }
    
    void AddComponentToRegistry(EID component_id, size_t size_per_component);
    template<class T>
        static EID RegisterComponent()
    {
        static_assert(std::is_base_of<IComponent, T>::value, "Custom components must inherit from IComponent.");
        static Component<T> new_component;
        AddComponentToRegistry(new_component.GetStaticId(), sizeof(T));
        return new_component.GetStaticId();
    }
    
    template<class T>
        static EID GetComponentId()
    {
        return Component<T>::STATIC_COMPONENT_ID;
    }
    
    // Returns a pointer to the component information
    // will be a list of data attached to each entity
    void* GetComponentFromRegistry(EID component_id);
    template<class T>
        static T* GetComponentData()
    {
        return (T*)GetComponentFromRegistry(Component<T>::STATIC_COMPONENT_ID);
    }
    
    // Returns a poitner to the compoment for the requested entity
    void* GetComponentFromRegistryForEntity(Entity entity, EID component_id, size_t size_of_component);
    template<class T>
        static T* GetComponentForEntity(Entity entity)
    {
        return (T*)GetComponentFromRegistryForEntity(entity, Component<T>::STATIC_COMPONENT_ID, sizeof(T));
    }
    
    
    // ComponentIter is a wrapper around the ComponentCache
    // to provide a clean interface for iterating over a list
    // of components.
    template <class T>
        struct ComponentIter
    {
        size_t next_index;
        T* next(bool swap = true);
    };
    
    template <class T>
        ComponentIter<T> GetComponentIter()
    {
        ComponentIter<T> iter = {};
        iter.next_index = 0;
        return iter;
    }
    
    void *NextInCache(EID component_id, size_t next_idx);
    void *NextInCacheNoSwap(EID component_id, size_t next_idx);
    template <class T>
        T* ComponentIter<T>::next(bool swap)
    {
        if (swap)
            return (T*)NextInCache(Component<T>::STATIC_COMPONENT_ID, next_index++);
        else
            return (T*)NextInCacheNoSwap(Component<T>::STATIC_COMPONENT_ID, next_index++);
    }
    
    void FlushComponentCache(EID component_id);
    template<class T>
        void FlushComponentCache()
    {
        FlushComponentCache(Component<T>::STATIC_COMPONENT_ID);
    }
    
} // ecs

#endif // JENGINE_ECS_COMPONENT_H
