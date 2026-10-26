#ifndef WORKITEM_H
#define WORKITEM_H

#include "WorkComponent.h"
#include "Colleague.h"

#include <string>

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
class WorkItem : public WorkComponent, public Colleague {
public:
    WorkItem();
    /**
     * @param name Display name of the task, e.g. "Check Documents".
     * @param id   Identifies the task within its definition. Clones keep the same ID,
     *             so instance ID + task ID identify a task in a running instance.
     */
    explicit WorkItem(const std::string& name, const std::string& id = std::string());
    virtual ~WorkItem();

    // Identity (copied by clone(), because clone() copy-constructs the task)
    void setName(const std::string& name);
    void setId(const std::string& id);
    const std::string& getName() const;
    const std::string& getId() const;

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

    void receive(const std::string& event, Colleague* source) override;
    bool isComplete() const;

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
    // Display name and definition-level ID of the task.
    std::string name;
    std::string id;
};

#endif
