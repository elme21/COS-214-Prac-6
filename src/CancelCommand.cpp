#include "CancelCommand.h"
#include "WorkComponent.h"
#include <iostream>

CancelCommand::CancelCommand(WorkComponent* workComponent) 
    : item(workComponent), previousState("") {}

bool CancelCommand::execute() {
    if (!item) return false;

    previousState = item->getState();
    item->cancel();

    cout << "[CancelCommand] Cancelled work component.\n";
    return true;
}

void CancelCommand::undo() {
    if (!item) return;

    item->restoreState(previousState);
    cout << "[CancelCommand] Undid cancellation, restored state silently.\n";
}