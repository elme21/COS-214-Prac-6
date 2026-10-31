#ifndef AUDITDECORATOR_H
#define AUDITDECORATOR_H

#include <functional>
#include <string>
#include "WorkComponentDecorator.h"

// Records every lifecycle attempt on a task, including refused ones.
// The sink is wired by the builder (for example to the event publisher).
class AuditDecorator : public WorkComponentDecorator {
public:
    typedef std::function<void(const std::string&)> Sink;

    AuditDecorator(WorkComponent* component, const std::string& taskName, Sink sink);

    bool assignTo(Participant* participant) override;
    bool start() override;
    bool approve() override;
    bool reject() override;
    bool complete() override;
    bool cancel() override;
    bool escalate() override;
    WorkComponent* clone() const override;

private:
    bool record(const std::string& action, bool succeeded) const;

    std::string taskName;
    Sink sink;
};

#endif