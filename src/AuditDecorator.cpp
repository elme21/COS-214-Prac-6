#include "AuditDecorator.h"

AuditDecorator::AuditDecorator(WorkComponent* component, const std::string& name, Sink s)
    : WorkComponentDecorator(component), taskName(name), sink(s) {}

bool AuditDecorator::record(const std::string& action, bool succeeded) const {
    if (sink) {
        sink(taskName + ": " + action + (succeeded ? " succeeded" : " refused"));
    }
    return succeeded;
}

bool AuditDecorator::assignTo(Participant* p) { return record("assign", WorkComponentDecorator::assignTo(p)); }
bool AuditDecorator::start() { return record("start", WorkComponentDecorator::start()); }
bool AuditDecorator::approve() { return record("approve", WorkComponentDecorator::approve()); }
bool AuditDecorator::reject() { return record("reject", WorkComponentDecorator::reject()); }
bool AuditDecorator::complete() { return record("complete", WorkComponentDecorator::complete()); }
bool AuditDecorator::cancel() { return record("cancel", WorkComponentDecorator::cancel()); }
bool AuditDecorator::escalate() { return record("escalate", WorkComponentDecorator::escalate()); }

WorkComponent* AuditDecorator::clone() const {
    return new AuditDecorator(*this);
}
