#ifndef MONITORINGCOLLECTOR_H
#define MONITORINGCOLLECTOR_H

#include <map>
#include <string>
#include "Event.h"
#include "EventListener.h"

class WorkflowEventBus;

/** Observer - ConcreteObserver. Collects execution statistics for reports. */
class MonitoringCollector : public EventListener {
public:
    explicit MonitoringCollector(WorkflowEventBus* subject);
    ~MonitoringCollector();

    void update() override;

    int countOf(EventType type) const;
    int completedFor(const std::string& instanceId) const;
    int totalEvents() const;
    std::string report() const;

private:
    WorkflowEventBus*          subject;
    std::map<EventType, int>   counts;
    std::map<std::string, int> completedPerInstance;
    int                        total;
};

#endif