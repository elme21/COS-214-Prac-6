#include "EventPublisher.h"
#include <algorithm>

void EventPublisher::subscribe(EventType type, EventListener* listener) {
    if (listener == nullptr || isSubscribed(type, listener)) return;
    listenerList[type].push_back(listener);
}

void EventPublisher::unsubscribe(EventType type, EventListener* listener) {
    std::map<EventType, std::vector<EventListener*> >::iterator it = listenerList.find(type);
    if (it == listenerList.end()) return;
    std::vector<EventListener*>& v = it->second;
    v.erase(std::remove(v.begin(), v.end(), listener), v.end());
}

bool EventPublisher::isSubscribed(EventType type, EventListener* listener) const {
    std::map<EventType, std::vector<EventListener*> >::const_iterator it = listenerList.find(type);
    if (it == listenerList.end()) return false;
    return std::find(it->second.begin(), it->second.end(), listener) != it->second.end();
}

std::size_t EventPublisher::listenerCount(EventType type) const {
    std::map<EventType, std::vector<EventListener*> >::const_iterator it = listenerList.find(type);
    return it == listenerList.end() ? 0 : it->second.size();
}

void EventPublisher::publish(const Event& event) {
    std::map<EventType, std::vector<EventListener*> >::iterator it = listenerList.find(event.type);
    if (it == listenerList.end()) return;

    // Iterate over a copy: a listener may (un)subscribe while being notified.
    std::vector<EventListener*> snapshot = it->second;
    for (std::size_t i = 0; i < snapshot.size(); ++i) {
        // skip listeners that were unsubscribed by an earlier listener in this round
        if (isSubscribed(event.type, snapshot[i])) {
            snapshot[i]->update();
        }
    }
}
