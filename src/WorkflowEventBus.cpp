#include "WorkflowEventBus.h"

WorkflowEventBus::WorkflowEventBus() : subjectState(), hasState(false) {}

void WorkflowEventBus::setState(const Event& event) {
    publish(event);
}

const State* WorkflowEventBus::getState() const {
    return hasState ? &subjectState : nullptr;
}

void WorkflowEventBus::publish(const Event& event) {
    subjectState = event;
    hasState = true;
    EventPublisher::publish(event);
}
