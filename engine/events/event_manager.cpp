
// Subtract Event_Type_Cutom and Key_Unknown
file_global const i32 EVENT_LIST_SIZE = EVENT_TYPE_COUNT + KEY_Count - 2;
file_global DynamicArray<EventSubscriber> EventSubscriberList[EVENT_LIST_SIZE];

// TODO(Dustin): Hashtable of Custom Events

void InitEventManager()
{
    for (int i = 0; i < EVENT_LIST_SIZE; ++i)
    {
        EventSubscriberList[i] = DynamicArray<EventSubscriber>();
    }
}

void ShutdownEventManager()
{
    for (int i = 0; i < EVENT_LIST_SIZE; ++i)
    {
        EventSubscriberList[i].Reset();
    }
}

void SubscribeToEvent(Event event, void (*subscriber)(void *instance, Event event), void *inst)
{
    EventSubscriber subs = {};
    subs.inst            = inst;
    subs.subscriber_fn   = subscriber;
    
    if (event.Type == EVENT_TYPE_CUSTOM)
    { // NOTE(Dustin): Not yet implemented
    }
    else
    {
        i32 event_idx = event.Type;
        
        if (event.Type == EVENT_TYPE_ON_KEY_TYPE_PRESS)
        {
            // 0..6 are Event_Type indices (Count == 8)
            EventSubscriberList[EVENT_TYPE_COUNT - 2 + event.OnKeyTypePressEvent.Key].PushBack(subs);
        }
        else
        {
            // events should not have a value >= EVENT_TYPE_CUSTOM
            assert(event.Type < EVENT_TYPE_COUNT - 1);
            EventSubscriberList[event.Type].PushBack(subs);
        }
    }
}

void DispatchEvent(Event event)
{
    
    int event_idx = -1;
    int event_key_idx = -1;
    if (event.Type == EVENT_TYPE_CUSTOM)
    { // NOTE(Dustin): Not yet implemented
    }
    else
    {
        event_idx = event.Type;
        
        // Dispatch to general Event Press/Release Events
        u32 sub_count = EventSubscriberList[event.Type].size;
        for (int subscriber_idx = 0; subscriber_idx < sub_count; ++subscriber_idx)
        {
            EventSubscriber subscriber = EventSubscriberList[event.Type][subscriber_idx];
            subscriber.subscriber_fn(subscriber.inst, event);
        }
        
        // Dispatch to key-specific press/release events
        if (event.Type == EVENT_TYPE_ON_KEY_TYPE_PRESS)
        {
            sub_count = EventSubscriberList[EVENT_TYPE_COUNT - 2 + event.OnKeyTypePressEvent.Key].size;
            for (int subscriber_idx = 0; subscriber_idx < sub_count; ++subscriber_idx)
            {
                EventSubscriber subscriber =
                    EventSubscriberList[EVENT_TYPE_COUNT - 2 + event.OnKeyTypePressEvent.Key][subscriber_idx];
                subscriber.subscriber_fn(subscriber.inst, event);
            }
        }
    }
}
