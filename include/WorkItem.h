#ifndef WORKITEM_H
#define WORKITEM_H

#include "WorkComponent.h"
#include "Colleague.h"

#include <string>

class WorkItemState;
class WorkCoordinator;

/// @brief
/// The WorkItem class is the Context of the State pattern, the AbstractClass of the Template Method
/// pattern, a Leaf of the Composite pattern and a Colleague of the Mediator pattern.
/// Template Method: execute() is the final skeleton (checkReady -> onEnter -> perform -> recordResult);
/// subclasses supply the primitive operation perform() and may override the hooks onEnter() / recordResult().
/// State: all lifecycle operations delegate to the current state, which returns true if the transition
/// happened and false if it is not allowed. Only WorkItemState (friend) may change the state or assignee
/// during normal transitions; restoreState() is the silent back door used only by command undo.
class WorkItem : public WorkComponent, public Colleague {
public:
    /// @brief The default constructor. The work item starts in CreatedState with no assignee, name or ID.
    WorkItem();
    /// @brief The constructor. The work item starts in CreatedState with no assignee.
    /// @param name Display name of the task, e.g. "Check Documents".
    /// @param id   Identifies the task within its definition. Clones keep the same ID,
    ///             so instance ID + task ID identify a task in a running instance.
    explicit WorkItem(const std::string& name, const std::string& id = std::string());
    /// @brief The destructor. Deletes nothing: the state and the assignee are not owned.
    virtual ~WorkItem();

    // Identity (copied by clone(), because clone() copy-constructs the task)
    /// @brief sets the display name of the task
    /// @param name the new display name
    void setName(const std::string& name);
    /// @brief sets the ID of the task within its definition
    /// @param id the new ID
    void setId(const std::string& id);
    /// @brief gets the display name of the task
    /// @return the display name
    const std::string& getName() const;
    /// @brief gets the ID of the task within its definition (the same in every clone)
    /// @return the ID
    const std::string& getId() const;

    // Template Method skeleton - subclasses cannot override it.
    /// @brief the template method (final): if checkReady() is false it returns at once; otherwise it calls
    /// onEnter(), then perform(), then recordResult() with perform()'s result.
    void execute() override final;

    // Composite (leaf behaviour)
    /// @brief does nothing: a leaf has no children
    /// @param component ignored
    void add(WorkComponent* component) override;
    /// @brief does nothing: a leaf has no children
    /// @param component ignored
    void remove(WorkComponent* component) override;
    /// @brief a leaf has no children
    /// @param index ignored
    /// @return always nullptr
    WorkComponent* getChild(int index) override;
    /// @brief creates a DepthFirstIterator rooted at this work item, so it visits exactly this one item.
    /// The caller owns the iterator (must delete it) and must call reset() before the first hasNext()/next().
    /// @return a new iterator owned by the caller
    WorkIterator* createIterator() override;

    // State (delegating)
    /// @brief delegates to the current state's makeAvailable() (allowed only in CreatedState)
    /// @return true if the transition happened, false if the current state does not allow it
    bool makeAvailable() override;
    /// @brief delegates to the current state's assign() (allowed in Available, Rejected and Escalated)
    /// @param participant the participant to assign (non-owning); null is refused
    /// @return true if the transition happened, false if the current state does not allow it
    bool assignTo(Participant* participant) override;
    /// @brief delegates to the current state's start() (allowed only in AssignedState)
    /// @return true if the transition happened, false if the current state does not allow it
    bool start() override;
    /// @brief delegates to the current state's approve() (allowed only in InProgressState)
    /// @return true if the transition happened, false if the current state does not allow it
    bool approve() override;
    /// @brief delegates to the current state's complete() (allowed only in InProgressState)
    /// @return true if the transition happened, false if the current state does not allow it
    bool complete() override;
    /// @brief delegates to the current state's reject() (allowed only in InProgressState)
    /// @return true if the transition happened, false if the current state does not allow it
    bool reject() override;
    /// @brief delegates to the current state's cancel() (refused only in the final states)
    /// @return true if the transition happened, false if the current state does not allow it
    bool cancel() override;
    /// @brief delegates to the current state's escalate() (allowed in Assigned and InProgress)
    /// @return true if the transition happened, false if the current state does not allow it
    bool escalate() override;

    // Undo support
    /// @brief gets the current state
    /// @return the current shared state (non-owning, never null)
    const WorkItemState* getState() const override;
    /// @brief gets the current assignee
    /// @return the current assignee (non-owning), or nullptr if unassigned
    Participant* getAssignee() const override;
    /// @brief sets the state and assignee directly and silently: no transition rules, no hooks, no output.
    /// For command undo ONLY.
    /// @param state the state to restore (non-owning shared instance)
    /// @param assignee the assignee to restore (non-owning, may be nullptr)
    void restoreState(const WorkItemState* state, Participant* assignee) override;

    /// @brief called by the Mediator when a prerequisite changed: if a coordinator is set and its
    /// canBecomeAvailable() agrees, calls makeAvailable(). Does nothing without a coordinator.
    /// @param event the event that occurred (not used)
    /// @param source the colleague that changed (not used)
    void receive(const std::string& event, Colleague* source) override;
    /// @brief checks if the work item is finished (asks the current state; true only in CompletedState)
    /// @return true if the work item is complete, false otherwise
    bool isComplete() const;

protected:
    // Template Method hooks. Decorators wrap only the public operations.
    /// @brief first step of the template method: asks the current state's canExecute()
    /// @return true if execute() may continue (only in InProgressState), false otherwise
    bool checkReady() const;
    /// @brief hook run before perform(); the default prints the current state to std::cout
    virtual void onEnter();
    /// @brief the primitive operation each ConcreteClass must supply: does the task's actual work
    /// @return true if the work finished (recordResult() then completes the item), false if it must wait
    virtual bool perform() = 0;
    /// @brief hook run after perform(); the default calls complete() if finished, otherwise the item
    /// stays InProgress until a later command finishes it
    /// @param finished the result of perform()
    virtual void recordResult(bool finished);

private:
    /// @brief only WorkItemState may call the private setters below during normal transitions
    friend class WorkItemState;
    /// @brief sets the current state (called through WorkItemState::transition)
    /// @param next the shared state instance to move to (non-owning)
    void changeState(const WorkItemState* next);
    /// @brief sets the current assignee (called through WorkItemState::setAssignee)
    /// @param participant the new assignee (non-owning)
    void setAssignee(Participant* participant);

    // Current state. NON-OWNING: states are shared singletons, never deleted here.
    /// @brief The current state. NON-OWNING: states are shared singletons, never deleted here.
    const WorkItemState* state;
    // Current assignee. NON-OWNING: Participants are owned by the resource layer (Member 4).
    /// @brief The current assignee, or nullptr. NON-OWNING: Participants are owned by the resource layer (Member 4).
    Participant* assignee;
    // Display name and definition-level ID of the task.
    /// @brief The display name of the task
    std::string name;
    /// @brief The ID of the task within its definition (copied unchanged by clone())
    std::string id;
};

#endif
