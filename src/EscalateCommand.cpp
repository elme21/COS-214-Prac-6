/**
 * @file EscalateCommand.cpp
 * @brief Implementation of EscalateCommand.
 */
#include "EscalateCommand.h"
#include "WorkComponent.h"

EscalateCommand::EscalateCommand(WorkComponent* workComponent)
    : item(workComponent), previousState(nullptr), previousParticipant(nullptr) {}

bool EscalateCommand::execute() {
    if (!item) return false;
    previousState = item->getState();
    previousParticipant = item->getAssignee();
    return item->escalate();
}

void EscalateCommand::undo() {
    if (item) {
        item->restoreState(previousState, previousParticipant);
    }
}
