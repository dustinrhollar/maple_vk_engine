#ifndef JENGINE_ECS_SYSTEM_H
#define JENGINE_ECS_SYSTEM_H

/*

system.h manages a Sytstem Registry for the Entity Component
System. All Systems are assigned a unique identifier based on
their type. Any user created Systems must inherit from ISystem,
which is a wrapper around the boolean value "IsActive". If a
user attempts to register a system that does not inherit from
ISystem, a compile error is triggered.

When registering Systems, a user has the option for setting the
data for that system. Default behavior is that data is not
provided.

User API:

struct ISystem
- Parent of all Registered Systems.

void InitializeSystemRegistry();
void ShutdownSystemRegistry();
- Initializes and shuts down the SystemRegistry.

GUID RegisterSystem<T>(T *data = nullptr)
- Register a system in the registry. Optionally, a caller
  can provide the data to set for the system.

T* GetSystem<T>()
- Retrieves the requested system from the registry.

*/

namespace ecs {
    
    // Each component gets a unique id
    extern EID STATIC_SYSTEM_ID;
    // Generate an id for a new type of component
    template<class T>
        EID GenerateNewSystemID()
    {
        return STATIC_SYSTEM_ID++;
    }
    
    // Basic interface that all systems should inherit from.
    // It is a wrapper around a boolean that determines if a system
    // is active.
    class ISystem
    {
        public:
        bool IsActive;
    };
    
    // A static class that acts as a wrapper around a static id for system types.
    template<class T>
        class System
    {
        public:
        
        EID GetStaticId() {return STATIC_SYSTEM_ID;}
        
        static const EID STATIC_SYSTEM_ID;
    };
    
    template<class T>
        const EID System<T>::STATIC_SYSTEM_ID = GenerateNewSystemID<T>();
    
    // Initialize and shutdown the SystemRegistry
    void InitializeSystemRegistry();
    void ShutdownSystemRegistry();
    
    void AddSystemToRegistry(EID system_id, size_t size_of_system, void *data = nullptr);
    template<class T>
        static EID RegisterSystem(T *data = nullptr)
    {
        static_assert(std::is_base_of<ISystem, T>::value, "Custom systems must inherit from ISystem.");
        
        static System<T> new_system;
        AddSystemToRegistry(new_system.GetStaticId(), sizeof(T), data);
        return new_system.GetStaticId();
    }
    
    void *GetSystemFromRegistry(EID system_id);
    template<class T>
        T* GetSystem()
    {
        return (T*)GetSystemFromRegistry(System<T>::STATIC_SYSTEM_ID);
    }
    
} // ecs

#endif // JENGINE_ECS_SYSTEM_H
