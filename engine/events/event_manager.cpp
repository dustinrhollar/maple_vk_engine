
namespace event
{
    u64 STATIC_EVENT_ID = 0;
    
    file_internal DynamicArray<SubscriberList> EventSubscribers;
    
    file_internal void SubscriberListInit(SubscriberList *list, u32 cap = 0);
    file_internal void SubscriberListFree(SubscriberList *list);
    file_internal void SubscriberListPushBack(SubscriberList *list, EventSubscriber subscriber);
    
    file_internal void SubscriberListInit(SubscriberList *list, u32 cap)
    {
        list->Size = 0;
        list->Cap  = cap;
        list->Ptr = palloc<EventSubscriber>(cap);
    }
    
    file_internal void SubscriberListFree(SubscriberList *list)
    {
        list->Size = 0;
        list->Cap  = 0;
        if (list->Ptr) pfree(list->Ptr);
        list->Ptr = nullptr;
    }
    
    file_internal void SubscriberListPushBack(SubscriberList *list, EventSubscriber subscriber)
    {
        if (list->Size + 1 >= list->Cap)
        { // resize
            list->Cap = (list->Cap == 0) ? 10 : list->Cap * 2;
            list->Ptr = prealloc<EventSubscriber>(list->Ptr, list->Cap);
        }
        
        list->Ptr[list->Size++] = subscriber;
    }
    
    void ManagerInit()
    {
        u32 initial_cap = 10;
        
        EventSubscribers.Resize(initial_cap);
        
        for (int i = 0; i < EventSubscribers.cap; ++i)
        {
            SubscriberListInit(&EventSubscribers[i], 5);
        }
    }
    
    void ManagerShutdown()
    {
        for (int i = 0; i < EventSubscribers.cap; ++i)
        {
            SubscriberListFree(&EventSubscribers[i]);
        }
        
        EventSubscribers.Reset();
    }
    
    void Subscribe(u64 event_id, void *callback, void *inst)
    {
        EventSubscriber subs = {};
        subs.inst            = inst;
        subs.FnCallback      = callback;
        
        if (event_id >= EventSubscribers.cap)
        {
            EventSubscribers.Resize(EventSubscribers.cap * 2);
        }
        
        SubscriberListPushBack(&EventSubscribers[event_id], subs);
    }
    
    void Unsubscribe(u64 event_id, void *callback, void *inst)
    {
        SubscriberList *subscribers = &EventSubscribers[event_id];
        
        for (u32 i = 0; i < subscribers->Size; ++i)
        {
            if (subscribers->Ptr[i].FnCallback == callback && subscribers->Ptr[i].inst == inst)
            {
                // swap the element to be removed with the last element in the list
                // when responding to events, the order the events are called is
                // irrelevelant
                subscribers->Ptr[i] = subscribers->Ptr[subscribers->Size - 1];
                --subscribers->Size;
            }
        }
    }
    
    void RetrieveSubscribers(u64 event_id, SubscriberList **subscribers)
    {
        *subscribers = &EventSubscribers[event_id];
    }
};
