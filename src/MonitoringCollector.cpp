#include "MonitoringCollector.h"
#include "WorkflowEventBus.h"
#include <sstream>

MonitoringCollector::MonitoringCollector(WorkflowEventBus* s) : subject(s), total(0) {
    if (subject) {
        const std::vector<EventType>& all = allEventTypes();
        for (std::size_t i = 0; i < all.size(); ++i) subject->subscribe(all[i], this);
    }
}

MonitoringCollector::~MonitoringCollector() {
    if (subject) {
        const std::vector<EventType>& all = allEventTypes();
        for (std::size_t i = 0; i < all.size(); ++i) subject->unsubscribe(all[i], this);
    }
}

void MonitoringCollector::update() {
    const State* s = subject ? subject->getState() : nullptr;
    if (!s) return;

    ++counts[s->type];
    ++total;
    if (s->type == EventType::ITEM_COMPLETED) ++completedPerInstance[s->instanceId];
}

int MonitoringCollector::countOf(EventType type) const {
    std::map<EventType, int>::const_iterator it = counts.find(type);
    return it == counts.end() ? 0 : it->second;
}

int MonitoringCollector::completedFor(const std::string& instanceId) const {
    std::map<std::string, int>::const_iterator it = completedPerInstance.find(instanceId);
    return it == completedPerInstance.end() ? 0 : it->second;
}

int MonitoringCollector::totalEvents() const { return total; }

std::string MonitoringCollector::report() const {
    std::ostringstream out;
    out << "=== Monitoring report (" << total << " events) ===\n";
    for (std::map<EventType, int>::const_iterator it = counts.begin(); it != counts.end(); ++it) {
        out << "  " << eventTypeToString(it->first) << ": " << it->second << "\n";
    }
    return out.str();
}
