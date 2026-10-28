// SHARED CONTRACT - agreed with Members 1, 3, 5. Do not change without telling the team.
#ifndef WORKCOMPONENT_H
#define WORKCOMPONENT_H

class WorkIterator;
class WorkItemState;
class Participant;

/**
 * @brief Common interface for every piece of work in a workflow.
 *
 * Component of the Composite pattern (WorkStage = composite, WorkItem = leaf)
 * and of the Decorator pattern (WorkComponentDecorator wraps any WorkComponent).
 *
 * Lifecycle operations return true if the transition happened and false if it
 * is not allowed (or unsupported, e.g. on a WorkStage). CommandProcessor only
 * pushes a command onto its history when the operation returned true.
 */
class WorkComponent {
public:
    /** @brief Virtual destructor so derived objects are deleted correctly through a WorkComponent*. */
    virtual ~WorkComponent() {}

    // --- Composite / Template Method / Decorator / Prototype / Iterator ---

    /** @brief Runs this component. A stage forwards the call to its children. */
    virtual void execute() = 0;

    /**
     * @brief Adds a child to this component.
     * @param component The child to add.
     */
    virtual void add(WorkComponent* component) = 0;

    /**
     * @brief Removes a child from this component.
     * @param component The child to remove.
     */
    virtual void remove(WorkComponent* component) = 0;

    /**
     * @brief Gets the child at a position.
     *
     * Implementations must return nullptr for an out-of-range index, because
     * the iterators use this to detect the end of a child list.
     *
     * @param index Zero-based position of the child.
     * @return The child, or nullptr if index is out of range.
     */
    virtual WorkComponent* getChild(int index) = 0;

    /**
     * @brief Creates an iterator over this component and everything beneath it.
     * @return A new iterator. The caller must delete it.
     */
    virtual WorkIterator* createIterator() = 0;

    /**
     * @brief Makes a deep copy of this component (Prototype pattern).
     * @return A new component. The caller must delete it.
     */
    virtual WorkComponent* clone() const = 0;

    // --- Lifecycle (State pattern, exposed through the component) ---

    /**
     * @brief Moves the work to the available state, ready to be picked up.
     * @return True if the transition happened, false if it is not allowed or unsupported.
     */
    virtual bool makeAvailable() = 0;

    /**
     * @brief Assigns the work to a participant.
     * @param participant The participant who receives the work.
     * @return True if the transition happened, false if it is not allowed or unsupported.
     */
    virtual bool assignTo(Participant* participant) = 0;

    /**
     * @brief Starts the work.
     * @return True if the transition happened, false if it is not allowed or unsupported.
     */
    virtual bool start() = 0;

    /**
     * @brief Approves the work.
     * @return True if the transition happened, false if it is not allowed or unsupported.
     */
    virtual bool approve() = 0;

    /**
     * @brief Marks the work as completed.
     * @return True if the transition happened, false if it is not allowed or unsupported.
     */
    virtual bool complete() = 0;

    /**
     * @brief Rejects the work.
     * @return True if the transition happened, false if it is not allowed or unsupported.
     */
    virtual bool reject() = 0;

    /**
     * @brief Cancels the work.
     * @return True if the transition happened, false if it is not allowed or unsupported.
     */
    virtual bool cancel() = 0;

    /**
     * @brief Escalates the work to the next level.
     * @return True if the transition happened, false if it is not allowed or unsupported.
     */
    virtual bool escalate() = 0;

    // --- Undo support (Command / Memento) ---
    // restoreState is used ONLY by command undo: it sets both fields directly
    // and silently (no hooks, no transitions, no output).

    /**
     * @brief Gets the current state of this component.
     * @return The current state, or nullptr if the component has no state (e.g. a WorkStage).
     */
    virtual const WorkItemState* getState() const = 0;

    /**
     * @brief Gets the participant this work is assigned to.
     * @return The assignee, or nullptr if there is none.
     */
    virtual Participant* getAssignee() const = 0;

    /**
     * @brief Restores an earlier state and assignee when a command is undone.
     *
     * Sets both fields directly and silently.
     *
     * @param state The state to restore.
     * @param assignee The assignee to restore.
     */
    virtual void restoreState(const WorkItemState* state, Participant* assignee) = 0;
};

#endif