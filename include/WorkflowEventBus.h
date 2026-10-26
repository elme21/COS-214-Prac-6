#ifndef WORKFLOWEVENTBUS_H
#define WORKFLOWEVENTBUS_H

#include "EventPublisher.h"

/**
 * Observer pattern - ConcreteSubject (extends EventPublisher, as in the diagram).
 * Keeps the latest event as subjectState. Listeners are told "something
 * changed" via update() and then read the state with getState().
 */
class WorkflowEventBus : public EventPublisher {
public:
    WorkflowEventBus();

    /** Stores 'event' as the current state, then notifies listeners of its type. */
    void setState(const Event& event);

    /** Latest state, or nullptr if nothing has been published yet. */
    const State* getState() const;

    /** Same as setState(): publishing through the bus always records the state first. */
    void publish(const Event& event) override;

private:
    State subjectState;
    bool  hasState;
};

#endif
