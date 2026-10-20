#include "CompleteCommand.h"
#include "WorkComponent.h"
#include <iostream>

CompleteCommand::CompleteCommand(WorkComponent* workComponent) 
    : item(workComponent), previousState("") {}

bool CompleteCommand::execute() {
    if (!item) return false;

    previousState = item->getState();
    item->complete(); 

    cout << "[CompleteCommand] Marked work component as complete.\n";
    return true;
}

void CompleteCommand::undo() {
    if (!item) return;

    // Restore state silently 
    item->restoreState(previousState);
    cout << "[CompleteCommand] Undid completion, restored state silently to: " << previousState << "\n";
}