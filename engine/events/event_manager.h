#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

// Something that listens to this event
struct EventSubscriber
{
    // callback function
    // Optional arguments: void *instance - instance of a class, struct, etc.
    void (*subscriber_fn)(void *instance, Event event);
    
    // instance point
    void *inst;
};


// Initialize the event manager. Creates a Subscriber list based on the
// number of possible event types. These types can be found in engine_events.h
// in the "EventType" and "EventKey" structs.
void InitEventManager();
void ShutdownEventManager();

// Subscribe a callback function to an event. The function should have the following signature:
//     void *instance: instance of a class/struct/etc. so that you can manipulate state if desired
//     Event event:    Event to subscribe to. It is important that the type field in the struct
//                     is set. For KeyType events, the key field in OnKeyType in OnKeyTypePressEvent
//                     must be set.
void SubscribeToEvent(Event event, void (*subscriber)(void *instance, Event event), void *inst);

// Dispatch an event - notifying all subscribers that the event has been updated.
void DispatchEvent(Event event);

#endif //EVENT_MANAGER_H
