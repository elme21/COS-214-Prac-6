#ifndef EVENTLISTENER_H
#define EVENTLISTENER_H

/**
 * Observer pattern - Observer.
 * Matches the class diagram: update() takes no arguments. The listener pulls
 * what happened from its subject with subject->getState().
 */
class EventListener {
public:
    virtual ~EventListener() {}
    virtual void update() = 0;
};

#endif