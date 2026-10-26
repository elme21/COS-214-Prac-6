#include "InProgressState.h"
#include "CancelledState.h"
#include "CompletedState.h"
#include "EscalatedState.h"
#include "RejectedState.h"

WorkItemState& InProgressState::instance() {
    static InProgressState theInstance;
    return theInstance;
}

bool InProgressState::complete(WorkItem& item) const { return transition(item, CompletedState::instance()); }

bool InProgressState::approve(WorkItem& item) const { return transition(item, CompletedState::instance()); }

bool InProgressState::reject(WorkItem& item) const { return transition(item, RejectedState::instance()); }

bool InProgressState::escalate(WorkItem& item) const { return transition(item, EscalatedState::instance()); }

bool InProgressState::cancel(WorkItem& item) const { return transition(item, CancelledState::instance()); }

bool InProgressState::canExecute() const { return true; }

std::string InProgressState::name() const { return "InProgress"; }
