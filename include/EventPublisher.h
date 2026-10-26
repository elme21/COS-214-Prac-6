#ifndef EVENTPUBLISHER_H
#define EVENTPUBLISHER_H

#include <map>
#include <vector>
#include <cstddef>
#include "Event.h"
#include "EventListener.h"

/**
 * Observer pattern - Subject.
 * Listeners subscribe per EventType. publish() notifies every listener
 * registered for that type, without knowing what they do.
 *
 * Publisher does NOT own the listeners (aggregation in the class diagram).
 */
class EventPublisher {
public:
    virtual ~EventPublisher() {}

    /** Register 'listener' for events of 'type'. Duplicate subscriptions are ignored. */
    void subscribe(EventType type, EventListener* listener);

    /** Remove 'listener' from 'type'. Safe to call if it was never subscribed. */
    void unsubscribe(EventType type, EventListener* listener);

    /** Notify all listeners registered for event.type. */
    virtual void publish(const Event& event);

    std::size_t listenerCount(EventType type) const;

protected:
    bool isSubscribed(EventType type, EventListener* listener) const;

    std::map<EventType, std::vector<EventListener*> > listenerList;
};

#endif
