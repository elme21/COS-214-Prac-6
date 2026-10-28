#ifndef DEADLINEESCALATOR_H
#define DEADLINEESCALATOR_H

#include <cstddef>
#include <ctime>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "Event.h"
#include "EventListener.h"

class WorkflowEventBus;

/**
 * Observer - ConcreteObserver.
 * Tracks deadlines. checkDeadlines(now) publishes DEADLINE_MISSED for overdue
 * items; on hearing DEADLINE_MISSED it runs the escalation action once per item.
 * The action is a std::function so it can be wired to Person 4's ApprovalHandler
 * chain without this class depending on it.
 */
class DeadlineEscalator : public EventListener {
public:
    typedef std::function<void(const Event&)> EscalationAction;

    explicit DeadlineEscalator(WorkflowEventBus* subject);
    ~DeadlineEscalator();

    void update() override;

    void setEscalationAction(EscalationAction action);
    void trackDeadline(const std::string& itemId, const std::string& instanceId, std::time_t deadline);
    void setDefaultTimeout(int seconds);   ///< 0 = don't auto-track available items
    int  checkDeadlines(std::time_t now);  ///< returns how many items were overdue

    std::size_t trackedCount() const;
    const std::vector<std::string>& getEscalated() const;

private:
    struct Tracked { std::string instanceId; std::time_t deadline; };
    typedef void (DeadlineEscalator::*Handler)(const Event&);

    void onAvailable(const Event& e);
    void onFinished(const Event& e);
    void onMissed(const Event& e);

    WorkflowEventBus*                  subject;
    std::map<EventType, Handler>       handlers;   // event type -> reaction (no switch)
    std::map<std::string, Tracked>     tracked;    // itemId -> deadline
    std::vector<std::string>           escalated;
    EscalationAction                   action;
    int                                defaultTimeout;
};

#endif
