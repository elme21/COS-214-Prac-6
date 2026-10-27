#include "MakeAvailableCommand.h"
#include "WorkComponent.h"

MakeAvailableCommand::MakeAvailableCommand(WorkComponent* workComponent)
    : item(workComponent), previousState(nullptr), previousParticipant(nullptr) {}

bool MakeAvailableCommand::execute() {
    if (!item) return false;
    previousState = item->getState();
    previousParticipant = item->getAssignee();
    return item->makeAvailable();
}

void MakeAvailableCommand::undo() {
    if (item) {
        item->restoreState(previousState, previousParticipant);
    }
}
