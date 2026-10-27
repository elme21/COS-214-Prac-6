#include "DependencyActivator.h"
#include "WorkflowEventBus.h"
#include "WorkCoordinator.h"   // Person 4
#include "WorkItem.h"          // Person 2
#include <algorithm>

DependencyActivator::DependencyActivator(WorkflowEventBus* s, WorkCoordinator* c)
    : subject(s), coordinator(c) {
    if (subject) {
        subject->subscribe(EventType::ITEM_COMPLETED, this);
        subject->subscribe(EventType::EXTERNAL_EVENT, this);
    }
}

DependencyActivator::~DependencyActivator() {
    if (subject) {
        subject->unsubscribe(EventType::ITEM_COMPLETED, this);
        subject->unsubscribe(EventType::EXTERNAL_EVENT, this);
    }
}

void DependencyActivator::addWaitingItem(WorkItem* item) {
    if (item && std::find(waiting.begin(), waiting.end(), item) == waiting.end()) {
        waiting.push_back(item);
    }
}

std::size_t DependencyActivator::waitingCount() const { return waiting.size(); }

void DependencyActivator::update() {
    // Only subscribed to ITEM_COMPLETED / EXTERNAL_EVENT, so any state we are told about is relevant.
    if (!coordinator || !subject || !subject->getState()) return;

    // Copy: activating an item may cause new events that modify 'waiting'.
    std::vector<WorkItem*> candidates = waiting;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        WorkItem* item = candidates[i];
        if (coordinator->canBecomeAvailable(item)) {
            waiting.erase(std::remove(waiting.begin(), waiting.end(), item), waiting.end());
            item->makeAvailable();   // INTEGRATION POINT with Person 2's WorkItem
        }
    }
}
