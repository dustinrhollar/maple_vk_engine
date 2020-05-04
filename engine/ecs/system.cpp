
namespace ecs {
    
    EID STATIC_SYSTEM_ID = 0;
    
    struct SystemElement
    {
        void *system = nullptr;
        size_t size_of_system = 0;
    };
    
    file_global SystemElement *SystemRegistry = nullptr;
    file_global size_t SystemCount = 0;
    
    file_global size_t DEFAULT_SYSTEM_CAPACITY = 10;
    file_global size_t SystemCapacity = 0;
    
    void InitializeSystemRegistry()
    {
        SystemCapacity = DEFAULT_SYSTEM_CAPACITY;
        SystemRegistry = (SystemElement*)mm::jalloc(SystemCapacity * sizeof(SystemElement));
    }
    
    void ShutdownSystemRegistry()
    {
        for (u32 i = 0; i < SystemCount; ++i)
            mm::jfree(SystemRegistry[i].system);
        
        mm::jfree(SystemRegistry);
        SystemCount = 0;
        SystemCapacity = 0;
    }
    
    void AddSystemToRegistry(EID system_id, size_t size_of_system, void *data)
    {
        // In case a user accidentally registers the same component twice
        // if (SystemRegistry[system_id].system != nullptr) return;
        // Each new component should be incremental in Id. So it should be added at "size"
        //assert(system_id == SystemCount);
        
        if (system_id >= SystemCapacity)
        { // Resize the registry
            size_t new_cap = SystemCapacity * 2; // amoritize add
            
            SystemElement *ptr = (SystemElement*)mm::jalloc(new_cap * sizeof(SystemElement));
            for (u32 i = 0; i < SystemCount; ++i)
            { // copy over the pointer to the memory blocks
                memcpy(&ptr[i], &SystemRegistry[i], sizeof(SystemElement));
            }
            
            mm::jfree(SystemRegistry);
            SystemRegistry = ptr;
            SystemCapacity = new_cap;
        }
        
        SystemRegistry[system_id].system = mm::jalloc(size_of_system);
        if (data)
        {
            memcpy(SystemRegistry[system_id].system, data, size_of_system);
        }
        SystemRegistry[system_id].size_of_system = size_of_system;
        
        // by default, systems are set to be active
        ((ISystem*)SystemRegistry[system_id].system)->IsActive = true;
        
        //SystemCount++;
    }
    
    void *GetSystemFromRegistry(EID system_id)
    {
        return SystemRegistry[system_id].system;
    }
} // ecs
