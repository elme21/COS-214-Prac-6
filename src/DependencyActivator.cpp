/**
 * @file DependencyActivator.cpp
 * @brief Implementation of DependencyActivator.
 *
 * @details update() is a single call into the Mediator via Colleague::changed(); all dependency logic stays in the WorkCoordinator.
 *
 * @author Logan (Observer and Strategy)
 * @date 2026-10-28
 * 
 */
#include "DependencyActivator.h"
#include "WorkflowEventBus.h"
#include "WorkItem.h"

DependencyActivator::DependencyActivator(WorkflowEventBus* s) : subject(s) {
    if (subject) subject->subscribe(EventType::ITEM_COMPLETED, this);
}

DependencyActivator::~DependencyActivator() {
    if (subject) subject->unsubscribe(EventType::ITEM_COMPLETED, this);
}

void DependencyActivator::update() {
    const State* s = subject ? subject->getState() : nullptr;
    if (!s || !s->item) return;

    // Colleague -> WorkCoordinator::notifyChanged -> every colleague's receive().
    s->item->changed("completed");
}
