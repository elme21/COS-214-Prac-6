/**
 * @file CancelCommand.cpp
 * @brief Implementation of CancelCommand.
 */
#include "CancelCommand.h"
#include "WorkComponent.h"

CancelCommand::CancelCommand(WorkComponent* workComponent)
    : item(workComponent), previousState(nullptr), previousParticipant(nullptr) {}

bool CancelCommand::execute() {
    if (!item) return false;
    previousState = item->getState();
    previousParticipant = item->getAssignee();
    return item->cancel();
}

void CancelCommand::undo() {
    if (item) {
        item->restoreState(previousState, previousParticipant);
    }
}
