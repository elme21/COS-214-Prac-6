#include "CompletedState.h"

WorkItemState& CompletedState::instance() {
    static CompletedState theInstance;
    return theInstance;
}

std::string CompletedState::name() const { return "Completed"; }
