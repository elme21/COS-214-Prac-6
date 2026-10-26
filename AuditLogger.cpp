#include "AuditLogger.h"
#include "WorkflowEventBus.h"

AuditLogger::AuditLogger(WorkflowEventBus* s) : subject(s) {
    if (subject) {
        const std::vector<EventType>& all = allEventTypes();
        for (std::size_t i = 0; i < all.size(); ++i) subject->subscribe(all[i], this);
    }
}

AuditLogger::~AuditLogger() {
    if (subject) {
        const std::vector<EventType>& all = allEventTypes();
        for (std::size_t i = 0; i < all.size(); ++i) subject->unsubscribe(all[i], this);
    }
}

void AuditLogger::update() {
    const State* s = subject ? subject->getState() : nullptr;
    if (!s) return;

    std::string line = "[AUDIT] " + eventTypeToString(s->type) +
                       " instance=" + s->instanceId + " item=" + s->itemId;
    if (!s->detail.empty()) line += " detail=\"" + s->detail + "\"";
    log.push_back(line);
}

const std::vector<std::string>& AuditLogger::getLog() const { return log; }