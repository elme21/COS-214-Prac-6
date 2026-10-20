#ifndef AUDITLOGGER_H
#define AUDITLOGGER_H

#include <string>
#include <vector>
#include "EventListener.h"

class WorkflowEventBus;

/** Observer - ConcreteObserver. Writes one audit line for every event. */
class AuditLogger : public EventListener {
public:
    explicit AuditLogger(WorkflowEventBus* subject);
    ~AuditLogger();

    void update() override;

    /** The audit trail so far (used by the TUI "view history" option). */
    const std::vector<std::string>& getLog() const;

private:
    WorkflowEventBus*        subject;
    std::vector<std::string> log;
};

#endif