/**
 * @file CompleteCommand.cpp
 * @brief Implementation of CompleteCommand.
 */
#include "CompleteCommand.h"
#include "WorkComponent.h"

CompleteCommand::CompleteCommand(WorkComponent* workComponent)
    : item(workComponent), previousState(nullptr), previousParticipant(nullptr) {}

bool CompleteCommand::execute() {
    if (!item) return false;
    previousState = item->getState();
    previousParticipant = item->getAssignee();
    return item->complete();
}

void CompleteCommand::undo() {
    if (item) {
        item->restoreState(previousState, previousParticipant);
    }
}
