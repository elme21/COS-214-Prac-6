#ifndef WORKITEMSTATE_H
#define WORKITEMSTATE_H

#include <string>

class WorkItem;
class Participant;

/**
 * State pattern - abstract state.
 *
 * Every operation returns true if the transition happened and false if it is
 * not allowed. The base implementation refuses everything; each concrete state
 * overrides only the operations it allows.
 *
 * States are stateless and shared: each concrete state exposes
 * static WorkItemState& instance(). The assignee lives on the WorkItem.
 */
class WorkItemState {
public:
    virtual ~WorkItemState() {}

    virtual bool makeAvailable(WorkItem& item) const;
    virtual bool assign(WorkItem& item, Participant* participant) const;
    virtual bool start(WorkItem& item) const;
    virtual bool approve(WorkItem& item) const;
    virtual bool complete(WorkItem& item) const;
    virtual bool reject(WorkItem& item) const;
    virtual bool cancel(WorkItem& item) const;
    virtual bool escalate(WorkItem& item) const;

    virtual std::string name() const = 0;
    virtual bool canExecute() const;

protected:
    WorkItemState() {}

    // WorkItem befriends WorkItemState only, and friendship is not inherited,
    // so concrete states reach WorkItem's private setters through these.
    static bool transition(WorkItem& item, const WorkItemState& next);
    static void setAssignee(WorkItem& item, Participant* participant);

private:
    WorkItemState(const WorkItemState&);
    WorkItemState& operator=(const WorkItemState&);
};

#endif
