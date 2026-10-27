#include "ApproveCommand.h"
#include "WorkComponent.h"

ApproveCommand::ApproveCommand(WorkComponent* workComponent)
    : item(workComponent), previousState(nullptr), previousParticipant(nullptr) {}

bool ApproveCommand::execute() {
    if (!item) return false;
    previousState = item->getState();
    previousParticipant = item->getAssignee();
    return item->approve();
}

void ApproveCommand::undo() {
    if (item) {
        item->restoreState(previousState, previousParticipant);
    }
}
