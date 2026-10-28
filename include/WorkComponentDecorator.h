/**
 * @file WorkComponentDecorator.h
 * @brief Abstract base class of the Decorator pattern for work components.
 */

#ifndef WORKCOMPONENTDECORATOR_H
#define WORKCOMPONENTDECORATOR_H

#include <memory>
#include "WorkComponent.h"

/**
 * @brief Wraps a WorkComponent, owns it, and forwards every operation to it.
 *
 * Concrete decorators override only the operations they hook, plus clone().
 * A decorated task is still a WorkComponent, so the Composite, Iterator and
 * Command code never needs to know whether a task is decorated.
 *
 * @par Design pattern
 * Decorator (Decorator role). It also takes part in Prototype: clone()
 * deep-copies the whole chain of wrapped components.
 *
 * @note getState(), getAssignee() and restoreState() are forwarded untouched
 *       on purpose, so undoing a command never re-triggers decorator hooks.
 */
class WorkComponentDecorator : public WorkComponent {
public:
    /**
     * @brief Wraps @p component.
     * @param component Component to wrap. The decorator takes ownership.
     * @throws std::invalid_argument if @p component is null.
     */
    explicit WorkComponentDecorator(WorkComponent* component);

    /**
     * @brief Deep copy: the wrapped component is cloned, not shared.
     * @param other Decorator to copy.
     */
    WorkComponentDecorator(const WorkComponentDecorator& other);

    /** @brief Not assignable; copy through clone() instead. */
    WorkComponentDecorator& operator=(const WorkComponentDecorator&) = delete;

    /** @brief Destroys the wrapped component. */
    virtual ~WorkComponentDecorator();

    /**@brief Provides access to the component wrapped by this decorator.*/
    /** @return A pointer to the wrapped WorkComponent without transferring ownership.*/
    WorkComponent* getWrappedComponent() const;

    /** @name Forwarded operations
     *  Passed unchanged to the wrapped component.
     *  @{ */

    void execute() override;                           ///< Forwards execute()
    void add(WorkComponent* child) override;           ///< Forwards add()
    void remove(WorkComponent* child) override;        ///< Forwards remove()
    WorkComponent* getChild(int index) override;       ///< Forwards getChild()
    WorkIterator* createIterator() override;           ///< Forwards createIterator()
 
    bool makeAvailable() override;                     ///< Forwards makeAvailable()
    bool assignTo(Participant* participant) override;  ///< Forwards assignTo()
    bool start() override;                             ///< Forwards start()
    bool approve() override;                           ///< Forwards approve()
    bool complete() override;                          ///< Forwards complete()
    bool reject() override;                            ///< Forwards reject()
    bool cancel() override;                            ///< Forwards cancel()
    bool escalate() override;                          ///< Forwards escalate()

    int getPriority() const override;  ///< Forwards getPriority().

    const WorkItemState* getState() const override;    ///< Forwards getState().
    Participant* getAssignee() const override;            ///< Forwards getAssignee().
    /** Forwards restoreState(); never triggers decorator hooks. */
    void restoreState(const WorkItemState* state, Participant* assignee) override;
    /** @} */
    // clone() stays pure: each concrete decorator copies itself.

protected:
    /** @return The wrapped component (still owned by this decorator). */
    WorkComponent* inner() const { return wrapped.get(); }

private:
    std::unique_ptr<WorkComponent> wrapped;    ///< The decorated component (owned).
};

#endif
