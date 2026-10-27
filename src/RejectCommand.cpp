#include "RejectCommand.h"
#include "WorkComponent.h"

RejectCommand::RejectCommand(WorkComponent* workComponent)
    : item(workComponent), previousState(nullptr), previousParticipant(nullptr) {}

bool RejectCommand::execute() {
    if (!item) return false;
    previousState = item->getState();
    previousParticipant = item->getAssignee();
    return item->reject();
}

void RejectCommand::undo() {
    if (item) {
        item->restoreState(previousState, previousParticipant);
    }
}
