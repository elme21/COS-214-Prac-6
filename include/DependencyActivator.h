#ifndef DEPENDENCYACTIVATOR_H
#define DEPENDENCYACTIVATOR_H

#include "EventListener.h"

class WorkflowEventBus;

/**
 * @file DependencyActivator.h
 * @brief Concrete observer that lets the Mediator re-evaluate dependencies.
 *
 * @details Bridges the Observer and Mediator patterns: an event from the bus becomes a call to
 * Colleague::changed(), which the WorkCoordinator (Mediator) distributes to every work item.
 *
 * @author Logan (Observer and Strategy)
 * @date 2026-10-28
 * @ingroup ObserverPattern
 */

/**
 * @brief ConcreteObserver: when a work item completes, tells the Mediator.
 *
 * It calls Colleague::changed("completed") on the finished item. That reaches
 * the WorkCoordinator (Mediator), which notifies every colleague. Each waiting
 * WorkItem::receive() then asks canBecomeAvailable() and makes itself available.
 * So this class never needs to know which items are waiting.
 *
 * @par Sequence
 * ITEM_COMPLETED published -> update() -> item->changed("completed") -> WorkCoordinator::notifyChanged()
 * -> each colleague's receive() -> canBecomeAvailable() -> makeAvailable().
 *
 * @pre The published event carries the completed item (Event::item) and the items are registered with the coordinator.
 * @note Events without an item are ignored.
 *
 * 
 * @see WorkCoordinator, WorkItem::receive()
 */
class DependencyActivator : public EventListener {
public:
    /// @brief Subscribes to ITEM_COMPLETED on the bus.
    /// @param subject the event bus to listen to (not owned, must outlive this object)
    /// @post Subscribed to EventType::ITEM_COMPLETED.
    explicit DependencyActivator(WorkflowEventBus* subject);

    /// @brief Unsubscribes from the bus.
    ~DependencyActivator();

    /// @brief Reads the completed item from the bus and notifies the Mediator through it.
    ///        Does nothing if the event carries no item.
    /// @see EventListener::update()
    void update() override;

private:
    /// @brief The event bus this listener is attached to (not owned).
    WorkflowEventBus* subject;
};

#endif
