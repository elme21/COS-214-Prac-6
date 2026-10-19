#include "AvailableState.h"
#include "AssignedState.h"
#include "CancelledState.h"

WorkItemState& AvailableState::instance() {
    static AvailableState theInstance;
    return theInstance;
}

bool AvailableState::assign(WorkItem& item, Participant* participant) const {
    if (participant == nullptr) return false;
    setAssignee(item, participant);
    return transition(item, AssignedState::instance());
}

bool AvailableState::cancel(WorkItem& item) const { return transition(item, CancelledState::instance()); }

std::string AvailableState::name() const { return "Available"; }
