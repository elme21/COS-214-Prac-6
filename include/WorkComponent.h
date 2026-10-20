// SHARED CONTRACT - agreed with Members 1, 3, 5. Do not change without telling the team.
#ifndef WORKCOMPONENT_H
#define WORKCOMPONENT_H

class WorkIterator;
class WorkItemState;
class Participant;

/**
 * Component of the Composite pattern (WorkStage = composite, WorkItem = leaf)
 * and of the Decorator pattern (WorkComponentDecorator wraps any WorkComponent).
 *
 * Lifecycle operations return true if the transition happened and false if it
 * is not allowed (or unsupported, e.g. on a WorkStage). CommandProcessor only
 * pushes a command onto its history when the operation returned true.
 */
class WorkComponent {
public:
    virtual ~WorkComponent() {}

    // --- Composite / Template Method / Decorator / Prototype ---
    virtual void execute() = 0;
    virtual void add(WorkComponent* component) = 0;
    virtual void remove(WorkComponent* component) = 0;
    virtual WorkComponent* getChild(int index) = 0;
    virtual WorkIterator* createIterator() = 0;
    virtual WorkComponent* clone() const = 0;

    // --- Lifecycle (State pattern, exposed through the component) ---
    virtual bool makeAvailable() = 0;
    virtual bool assignTo(Participant* participant) = 0;
    virtual bool start() = 0;
    virtual bool approve() = 0;
    virtual bool complete() = 0;
    virtual bool reject() = 0;
    virtual bool cancel() = 0;
    virtual bool escalate() = 0;

    // --- Undo support (Command / Memento) ---
    // restoreState is used ONLY by command undo: it sets both fields directly
    // and silently (no hooks, no transitions, no output).
    virtual const WorkItemState* getState() const = 0;
    virtual Participant* getAssignee() const = 0;
    virtual void restoreState(const WorkItemState* state, Participant* assignee) = 0;
};

#endif
