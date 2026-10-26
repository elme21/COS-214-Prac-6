#include "AssignedState.h"
#include "CancelledState.h"
#include "EscalatedState.h"
#include "InProgressState.h"

WorkItemState& AssignedState::instance() {
    static AssignedState theInstance;
    return theInstance;
}

bool AssignedState::start(WorkItem& item) const { return transition(item, InProgressState::instance()); }

bool AssignedState::escalate(WorkItem& item) const { return transition(item, EscalatedState::instance()); }

bool AssignedState::cancel(WorkItem& item) const { return transition(item, CancelledState::instance()); }

std::string AssignedState::name() const { return "Assigned"; }
