#ifndef WORKCOMPONENT_H
#define WORKCOMPONENT_H

class WorkItemState;
class Participant;
class WorkIterator;

// Shared contract for Composite, Decorator, Prototype, Command and State.
// Lifecycle operations return true when the transition happened and false
// when it was invalid or refused (for example by a decorator).
class WorkComponent {
public:
    virtual ~WorkComponent() {}

    virtual void execute() = 0;

    // Composite. add() takes ownership of the child.
    virtual void add(WorkComponent* child) = 0;
    virtual void remove(WorkComponent* child) = 0;
    virtual WorkComponent* getChild(int index) = 0;
    virtual WorkIterator* createIterator() = 0;

    // Prototype: deep copy of this component and everything it owns.
    virtual WorkComponent* clone() const = 0;

    // Lifecycle (WorkStage returns false for all of these).
    virtual bool makeAvailable() = 0;
    virtual bool assignTo(Participant* participant) = 0;
    virtual bool start() = 0;
    virtual bool approve() = 0;
    virtual bool complete() = 0;
    virtual bool reject() = 0;
    virtual bool cancel() = 0;
    virtual bool escalate() = 0;

    // Proposed: lets ordering and routing read priority without type checks.
    virtual int getPriority() const = 0;

    // Undo support. restoreState bypasses decorator hooks by design.
    virtual WorkItemState* getState() const = 0;
    virtual Participant* getAssignee() const = 0;
    virtual void restoreState(WorkItemState* state, Participant* assignee) = 0;
};

#endif
