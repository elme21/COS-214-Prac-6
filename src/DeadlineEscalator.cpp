#include "DeadlineEscalator.h"
#include "WorkflowEventBus.h"

DeadlineEscalator::DeadlineEscalator(WorkflowEventBus* s) : subject(s), defaultTimeout(0) {
    handlers[EventType::ITEM_AVAILABLE]  = &DeadlineEscalator::onAvailable;
    handlers[EventType::ITEM_COMPLETED]  = &DeadlineEscalator::onFinished;
    handlers[EventType::ITEM_CANCELLED]  = &DeadlineEscalator::onFinished;
    handlers[EventType::DEADLINE_MISSED] = &DeadlineEscalator::onMissed;

    if (subject) {
        for (std::map<EventType, Handler>::const_iterator it = handlers.begin(); it != handlers.end(); ++it) {
            subject->subscribe(it->first, this);
        }
    }
}

DeadlineEscalator::~DeadlineEscalator() {
    if (subject) {
        for (std::map<EventType, Handler>::const_iterator it = handlers.begin(); it != handlers.end(); ++it) {
            subject->unsubscribe(it->first, this);
        }
    }
}

void DeadlineEscalator::update() {
    const State* s = subject ? subject->getState() : nullptr;
    if (!s) return;

    State current = *s;   // copy: reacting could publish further events
    std::map<EventType, Handler>::const_iterator it = handlers.find(current.type);
    if (it != handlers.end()) (this->*(it->second))(current);
}

void DeadlineEscalator::onAvailable(const Event& e) {
    if (defaultTimeout > 0) trackDeadline(e.itemId, e.instanceId, e.timestamp + defaultTimeout);
}

void DeadlineEscalator::onFinished(const Event& e) { tracked.erase(e.itemId); }

void DeadlineEscalator::onMissed(const Event& e) {
    escalated.push_back(e.itemId);
    tracked.erase(e.itemId);             // escalate once only
    if (action) action(e);
}

void DeadlineEscalator::setEscalationAction(EscalationAction a) { action = a; }

void DeadlineEscalator::setDefaultTimeout(int seconds) { defaultTimeout = seconds; }

void DeadlineEscalator::trackDeadline(const std::string& itemId, const std::string& instanceId,
                                      std::time_t deadline) {
    Tracked t;
    t.instanceId = instanceId;
    t.deadline = deadline;
    tracked[itemId] = t;
}

std::size_t DeadlineEscalator::trackedCount() const { return tracked.size(); }

const std::vector<std::string>& DeadlineEscalator::getEscalated() const { return escalated; }

int DeadlineEscalator::checkDeadlines(std::time_t now) {
    // Collect first: publishing triggers onMissed(), which erases from 'tracked'.
    std::vector<std::pair<std::string, std::string> > overdue;
    for (std::map<std::string, Tracked>::const_iterator it = tracked.begin(); it != tracked.end(); ++it) {
        if (it->second.deadline <= now) overdue.push_back(std::make_pair(it->first, it->second.instanceId));
    }
    for (std::size_t i = 0; i < overdue.size(); ++i) {
        if (subject) subject->publish(Event(EventType::DEADLINE_MISSED, overdue[i].second, overdue[i].first));
    }
    return static_cast<int>(overdue.size());
}
