#ifndef WORKITEM_H
#define WORKITEM_H

#include "WorkComponent.h"

class WorkItemState;
class WorkCoordinator;

/**
 * Leaf of the Composite pattern, abstract class of the Template Method pattern
 * and context of the State pattern.
 *
 * Template Method: execute() is the final skeleton; subclasses supply perform()
 * and may override onEnter() / recordResult().
 *
 * State: all lifecycle operations delegate to the current state. Only
 * WorkItemState (friend) may change the state or assignee during normal
 * transitions; restoreState() is the silent back door used only by command undo.
 */
class WorkItem : public WorkComponent {
public:
    WorkItem();
    virtual ~WorkItem();

    // Template Method skeleton - subclasses cannot override it.
    void execute() override final;

    // Composite (leaf behaviour)
    void add(WorkComponent* component) override;
    void remove(WorkComponent* component) override;
    WorkComponent* getChild(int index) override;
    WorkIterator* createIterator() override;

    // State (delegating)
    bool makeAvailable() override;
    bool assignTo(Participant* participant) override;
    bool start() override;
    bool approve() override;
    bool complete() override;
    bool reject() override;
    bool cancel() override;
    bool escalate() override;

    // Undo support
    const WorkItemState* getState() const override;
    Participant* getAssignee() const override;
    void restoreState(const WorkItemState* state, Participant* assignee) override;

    // Mediator (Member 4) - stored but not used by this class.
    void setCoordinator(WorkCoordinator* coordinator);

protected:
    // Template Method hooks. Decorators wrap only the public operations.
    bool checkReady() const;
    virtual void onEnter();
    virtual bool perform() = 0;
    virtual void recordResult(bool finished);

private:
    friend class WorkItemState;
    void changeState(const WorkItemState* next);
    void setAssignee(Participant* participant);

    // Current state. NON-OWNING: states are shared singletons, never deleted here.
    const WorkItemState* state;
    // Current assignee. NON-OWNING: Participants are owned by the resource layer (Member 4).
    Participant* assignee;
    // Mediator. NON-OWNING: owned by Member 4's ResourceCoordinator.
    WorkCoordinator* coordinator;
};

#endif
