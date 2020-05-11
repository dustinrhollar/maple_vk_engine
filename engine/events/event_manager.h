#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

// event::Subscribe<T>(..)
// event::Dispatch<T>(..)

namespace event
{
    // Each component gets a unique id
    extern u64 STATIC_EVENT_ID;
    // Generate an id for a new type of component
    template<class T>
        EID GenerateNewEventID()
    {
        return STATIC_EVENT_ID++;
    }
    
    // A static class that acts as a wrapper around a static id for component types.
    template<class T>
        class EventIdManager
    {
        public:
        u64 GetStaticId() {return STATIC_EVENT_ID;}
        static const u64 STATIC_EVENT_ID;
    };
    
    template<class T>
        const u64 EventIdManager<T>::STATIC_EVENT_ID = GenerateNewEventID<T>();
    
    struct EventSubscriber
    {
        // callback function
        // Optional arguments: void *instance - instance of a class, struct, etc.
        void *FnCallback;
        
        // instance point
        void *inst;
    };
    
    struct SubscriberList
    {
        u32 Size;
        u32 Cap;
        EventSubscriber *Ptr;
    };
    
    void ManagerInit();
    void ManagerShutdown();
    
    void Subscribe(u64 event_id, void *callback, void *inst);
    template<class T>
        void Subscribe(void (*fn_callback)(void *instance, T event), void *inst)
    {
        Subscribe(EventIdManager<T>::STATIC_EVENT_ID, fn_callback, inst);
    }
    
    void RetrieveSubscribers(u64 event_id, SubscriberList **subscribers);
    template<class T>
        void Dispatch(T event)
    {
        SubscriberList *subscribers = nullptr;
        
        RetrieveSubscribers(EventIdManager<T>::STATIC_EVENT_ID, &subscribers);
        assert(subscribers != nullptr);
        
        for (int i = 0; i < subscribers->Size; ++i)
        {
            EventSubscriber subscriber = subscribers->Ptr[i];
            
            // NOTE(Dustin): Cast to the function
            void (*EventCallback)(void *instance, T event);
            
            EventCallback = (void (*)(void*, T))subscriber.FnCallback;
            EventCallback(subscriber.inst, event);
        }
    }
}; // event

#endif //EVENT_MANAGER_H
