#include "EscalatedState.h"
#include "AssignedState.h"
#include "CancelledState.h"

WorkItemState& EscalatedState::instance() {
    static EscalatedState theInstance;
    return theInstance;
}

bool EscalatedState::assign(WorkItem& item, Participant* participant) const {
    if (participant == nullptr) return false;
    setAssignee(item, participant);
    return transition(item, AssignedState::instance());
}

bool EscalatedState::cancel(WorkItem& item) const { return transition(item, CancelledState::instance()); }

std::string EscalatedState::name() const { return "Escalated"; }
