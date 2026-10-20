#ifndef NOTIFICATIONSENDER_H
#define NOTIFICATIONSENDER_H

#include <string>
#include <vector>
#include "EventListener.h"

class WorkflowEventBus;

/**
 * Observer - ConcreteObserver.
 * Builds an applicant-facing message for the events that matter to them
 * (which events those are is decided by a lookup table, not a switch).
 */
class NotificationSender : public EventListener {
public:
    explicit NotificationSender(WorkflowEventBus* subject);
    ~NotificationSender();

    void update() override;

    /** Messages produced so far. */
    const std::vector<std::string>& getSent() const;

private:
    WorkflowEventBus*        subject;
    std::vector<std::string> sent;
};

#endif