#include "StartCommand.h"
#include "WorkComponent.h"

StartCommand::StartCommand(WorkComponent* workComponent)
    : item(workComponent), previousState(nullptr), previousParticipant(nullptr) {}

bool StartCommand::execute() {
    if (!item) return false;
    previousState = item->getState();
    previousParticipant = item->getAssignee();
    return item->start();
}

void StartCommand::undo() {
    if (item) {
        item->restoreState(previousState, previousParticipant);
    }
}
