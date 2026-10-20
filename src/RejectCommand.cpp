#include "RejectCommand.h"
#include "WorkComponent.h"
#include <iostream>

RejectCommand::RejectCommand(WorkComponent* workComponent) 
    : item(workComponent), previousState("") {}

bool RejectCommand::execute() {
    if (!item) return false;

    previousState = item->getState();
    item->reject();

    cout << "[RejectCommand] Rejected work component.\n";
    return true;
}

void RejectCommand::undo() {
    if (!item) return;

    item->restoreState(previousState);
    cout << "[RejectCommand] Undid rejection, restored state silently.\n";
}