#ifndef WORKITEMSTATE_H
#define WORKITEMSTATE_H

#include <string>

class WorkItem;
class Participant;

/// @brief
/// The WorkItemState class is the State of the State pattern (WorkItem is the Context).
/// Every lifecycle operation returns true if the transition happened and false if the current state
/// does not allow it. The base implementation refuses everything; each ConcreteState overrides only
/// the operations it allows.
/// States are stateless and shared: each concrete state exposes static WorkItemState& instance(),
/// and a WorkItem holds a NON-OWNING const WorkItemState* to it. The assignee lives on the WorkItem.
class WorkItemState {
public:
    /// @brief The destructor
    virtual ~WorkItemState() {}

    /// @brief makes the work item available for assignment. Refused (returns false) unless overridden.
    /// @param item the work item whose state is changing
    /// @return true if the transition happened, false if this state does not allow it
    virtual bool makeAvailable(WorkItem& item) const;
    /// @brief assigns the work item to a participant. Refused (returns false) unless overridden.
    /// @param item the work item whose state is changing
    /// @param participant the participant to assign the work item to (non-owning)
    /// @return true if the transition happened, false if this state does not allow it
    virtual bool assign(WorkItem& item, Participant* participant) const;
    /// @brief starts work on the work item. Refused (returns false) unless overridden.
    /// @param item the work item whose state is changing
    /// @return true if the transition happened, false if this state does not allow it
    virtual bool start(WorkItem& item) const;
    /// @brief approves the work item. Refused (returns false) unless overridden.
    /// @param item the work item whose state is changing
    /// @return true if the transition happened, false if this state does not allow it
    virtual bool approve(WorkItem& item) const;
    /// @brief completes the work item. Refused (returns false) unless overridden.
    /// @param item the work item whose state is changing
    /// @return true if the transition happened, false if this state does not allow it
    virtual bool complete(WorkItem& item) const;
    /// @brief rejects the work item. Refused (returns false) unless overridden.
    /// @param item the work item whose state is changing
    /// @return true if the transition happened, false if this state does not allow it
    virtual bool reject(WorkItem& item) const;
    /// @brief cancels the work item. Refused (returns false) unless overridden.
    /// @param item the work item whose state is changing
    /// @return true if the transition happened, false if this state does not allow it
    virtual bool cancel(WorkItem& item) const;
    /// @brief escalates the work item. Refused (returns false) unless overridden.
    /// @param item the work item whose state is changing
    /// @return true if the transition happened, false if this state does not allow it
    virtual bool escalate(WorkItem& item) const;
    /// @brief checks if this state counts as finished for dependency checks (used by WorkItem::isComplete()
    /// and the Mediator). Only CompletedState returns true.
    /// @return true if the work item is complete, false otherwise
    virtual bool isComplete() const { return false; }

    /// @brief gets the name of the state, e.g. "InProgress"
    /// @return the name of the state
    virtual std::string name() const = 0;
    /// @brief checks if the work item may run its template method (WorkItem::checkReady() asks this).
    /// Only InProgressState returns true.
    /// @return true if execute() may run perform(), false otherwise
    virtual bool canExecute() const;

protected:
    /// @brief The constructor. Protected: only the concrete states create (one shared) instance each.
    WorkItemState() {}

    // WorkItem befriends WorkItemState only, and friendship is not inherited,
    // so concrete states reach WorkItem's private setters through these.
    /// @brief moves the work item to the next state
    /// @param item the work item whose state is changing
    /// @param next the shared state instance to move to
    /// @return always true, so a concrete state can return it as the result of an allowed transition
    static bool transition(WorkItem& item, const WorkItemState& next);
    /// @brief sets the assignee of the work item (used by the states that allow assign)
    /// @param item the work item being assigned
    /// @param participant the participant to assign (non-owning)
    static void setAssignee(WorkItem& item, Participant* participant);

private:
    /// @brief The copy constructor. Declared private and not defined: shared states cannot be copied.
    /// (Its parameter is unnamed, so it has no @param.)
    WorkItemState(const WorkItemState&);
    /// @brief The copy assignment operator. Declared private and not defined: shared states cannot be copied.
    /// (Its parameter is unnamed, so it has no @param.)
    /// @return a reference to this state
    WorkItemState& operator=(const WorkItemState&);
};

#endif
