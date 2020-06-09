
namespace ecs {
    
    file_global u16 EntityRegistry[MAX_ENTITIES];
    file_global u64 LastEntityIndex;
    file_global DynamicArray<EID> FreeIndices;
    file_global u32 MIN_FREE_INDICES = 1024;
    
    // Each entity gets a linked list of attached components
    file_global DynamicArray<EID> EntityComponents[MAX_ENTITIES];
    
    void IntializeEntityRegistry()
    {
        LastEntityIndex = 0;
        
        for (u32 i = 0; i < MAX_ENTITIES; ++i)
        {
            EntityComponents[i] = DynamicArray<EID>();
        }
    }
    
    void ShutdownEntityRegistry()
    {
        for (u32 i = 0; i < MAX_ENTITIES; ++i)
        {
            EntityRegistry[i] = 0; // reset all generations to 0
            
            EntityComponents[i].Reset();
        }
        
        LastEntityIndex = 0;
        
        FreeIndices.Reset();
    }
    
    void LoadEntityRegistryFromFile(jstring &entity_registry)
    {
        // File is written in the order of:
        // MaxEntities:     u32
        // LastEntityIndex: u64
        // EntityRegistry:  Array of u16
        // FreeIndices:     Linked List of u64
        // EntityComponent: Array of LinkedLists of u64
        
        jstring file_result = PlatformLoadFile(entity_registry);
        
        if (file_result.len > 0)
        {
            FileBuffer buffer = {};
            buffer.start = (file_result.heap) ? file_result.hptr : file_result.sptr;
            buffer.brkp  = buffer.start;
            buffer.cap   = file_result.len;
            
            u32 max_entities;
            ReadUInt32FromBinaryBuffer(&buffer, &max_entities);
            
            assert(max_entities == MAX_ENTITIES && "Entity Registry binary Max Entities does not match actual declaration!\n");
            
            ReadUInt64FromBinaryBuffer(&buffer, &LastEntityIndex);
            
            for (u32 i = 0; i < MAX_ENTITIES; ++i)
            {
                ReadUInt16FromBinaryBuffer(&buffer, &EntityRegistry[i]);
            }
            
            u32 free_idices_size;
            ReadUInt32FromBinaryBuffer(&buffer, &free_idices_size);
            for (u32 i = 0; i < free_idices_size; ++i)
            {
                EID result;
                ReadUInt64FromBinaryBuffer(&buffer, &result);
                
                FreeIndices.PushBack(result);
            }
            
            for (u32 i = 0; i < MAX_ENTITIES; ++i)
            {
                u32 component_count;
                ReadUInt32FromBinaryBuffer(&buffer, &component_count);
                
                for (u32 j = 0; j < component_count; ++j)
                {
                    EID result;
                    ReadUInt64FromBinaryBuffer(&buffer, &result);
                    
                    EntityComponents[i].PushBack(result);
                }
            }
        }
        
        file_result.Clear();
    }
    
    /*

NOTE(Dustin): When first writing this function, I noticed that
windows was adding in padding to numbers around 128. This ended
 up causing issues when reading from the file. Currently, the
 padding is still there, but the reader works just fine.

"What was the solution?" - you might ask. Beats me.

I just played around writing numbers to a file and it started working.
This has been the only file to cause this problem and i haven been
unable to replicate the issue. So a heads up - if you start reading
in large numbers from the entity binary it's probably because of this
error. Good luck fixing it because I have no idea what fixed it in the
first place.

*/
    void FlushEntityRegistryToFile(jstring &entity_registry)
    {
        // File is written in the order of:
        // MaxEntities:     u32
        // LastEntityIndex: u64
        // EntityRegistry:  Array of u16
        // FreeIndices:     Linked List of u64
        // EntityComponent: Array of LinkedLists of u64
        
        size_t estimated_size = MAX_ENTITIES * sizeof(u16) +
            MAX_ENTITIES * sizeof(DynamicArray<EID>) +
            FreeIndices.size * sizeof(DynamicArray<EID>);
        
        FileBuffer buffer;
        CreateFileBuffer(&buffer, estimated_size);
        
        u32 test = MAX_ENTITIES;
        UInt32ToBinaryBuffer(&buffer, &test, 1);
        
        //LastEntityIndex = MAX_ENTITIES+1;
        UInt64ToBinaryBuffer(&buffer, &LastEntityIndex, 1);
        
        // Write the generation list
        UInt16ToBinaryBuffer(&buffer, EntityRegistry, MAX_ENTITIES);
        
        // Write Linked List of Free Indices
        UInt32ToBinaryBuffer(&buffer, &FreeIndices.size, 1);
        for (u32 i = 0; i < FreeIndices.size; ++i)
        {
            UInt64ToBinaryBuffer(&buffer, &FreeIndices.Get(i), 1);
        }
        
        // Write List of Linked List of Component Ids
        for (u32 i = 0; i < MAX_ENTITIES; ++i)
        {
            UInt32ToBinaryBuffer(&buffer, &EntityComponents[i].size, 1);
            for (u32 j = 0; j < EntityComponents[i].size; ++j)
            {
                UInt64ToBinaryBuffer(&buffer, &EntityComponents[i][j], 1);
            }
        }
        
        PlatformWriteBufferToFile(entity_registry, buffer.start, buffer.brkp - buffer.start);
    }
    
    file_internal Entity GenerateEntity(u64 index, u64 generation)
    {
        Entity entity;
        entity.id = ((generation << ENTITY_INDEX_BITS)) | (index);
        return entity;
    }
    
    // Determines if the Entity is "alive"
    bool IsValidEntity(Entity entity)
    {
        u16 gen = EntityRegistry[entity.index()];
        return  gen == entity.generation();
    }
    
    void DestroyEntity(Entity entity)
    {
        if (IsValidEntity(entity))
        {
            u64 idx = entity.index();
            
            DynamicArray<EID> *components = &EntityComponents[idx];
            EID uid;
            while (components->Size() > 0)
            {
                components->Pop(uid);
                RemoveEntityFromComponent(uid, entity);
            }
            
            ++EntityRegistry[idx];
            FreeIndices.Push(idx);
        }
    }
    
    Entity CreateEntity()
    {
        u64 index;
        if (FreeIndices.Size() > MIN_FREE_INDICES)
        {
            FreeIndices.Pop(index);
        }
        else
        {
            u16 default_gen = 0;
            EntityRegistry[LastEntityIndex++] = (default_gen);
            index = LastEntityIndex-1;
            assert(index < (((u64)1)<<ENTITY_INDEX_BITS));
        }
        
        return GenerateEntity(index, EntityRegistry[index]);
    }
    
    void AttachComonentToEntity(Entity entity, EID component_id)
    {
        if (IsValidEntity(entity))
        {
            DynamicArray<EID> *components = &EntityComponents[entity.index()];
            
            if (IsComponentAttached(entity, component_id))
            {// duplicate found, no need to to keep searching or add
                return;
            }
            
            components->PushBack(component_id);
        }
    }
    
    void DetachComponentFromEntity(Entity entity, EID component_id)
    {
        if (IsValidEntity(entity))
        {
            DynamicArray<EID> *components = &EntityComponents[entity.index()];
            
            EID result;
            components->Remove(component_id, result);
        }
    }
    
    DynamicArray<EID> *GetAttachedComponents(Entity entity)
    {
        if (IsValidEntity(entity))
            return &EntityComponents[entity.index()];
        else
            return nullptr;
    }
    
    bool IsComponentAttached(Entity entity, EID component_id)
    {
        if (IsValidEntity(entity))
        {
            DynamicArray<EID> *components = &EntityComponents[entity.index()];
            for (u32 i = 0; i < components->size; ++i)
            {
                // Is this the component?
                if ((*components)[i] == component_id) return true;
            }
        }
        
        return false;
    }
    
    
    void EntityToBinaryBuffer(FileBuffer *buffer, ecs::Entity entity)
    {
        UInt64ToBinaryBuffer(buffer, &entity.id, 1);
    }
    
    void ReadEntityFromBinaryBuffer(FileBuffer *buffer, ecs::Entity *entity)
    {
        ReadUInt64FromBinaryBuffer(buffer, &entity->id);
    }
    
} // ecs
