#ifndef DEPENDENCYACTIVATOR_H
#define DEPENDENCYACTIVATOR_H

#include <cstddef>
#include <vector>
#include "EventListener.h"

class WorkflowEventBus;
class WorkCoordinator;   // Person 4 (Mediator)
class WorkItem;          // Person 2

/**
 * Observer - ConcreteObserver.
 * When an item completes or an external event arrives, asks the
 * WorkCoordinator which waiting items can now become available and activates them.
 */
class DependencyActivator : public EventListener {
public:
    DependencyActivator(WorkflowEventBus* subject, WorkCoordinator* coordinator);
    ~DependencyActivator();

    void update() override;

    /** Register an item that is waiting for its prerequisites. */
    void addWaitingItem(WorkItem* item);
    std::size_t waitingCount() const;

private:
    WorkflowEventBus*      subject;
    WorkCoordinator*       coordinator;
    std::vector<WorkItem*> waiting;
};

#endif
