#include "ApproveCommand.h"
#include "WorkComponent.h"
#include <iostream>

ApproveCommand::ApproveCommand(WorkComponent* workComponent) 
    : item(workComponent), previousState("") {}

bool ApproveCommand::execute() {
    if (!item) return false;

    // Save previous state before modifying
    previousState = item->getState();
    
    item->approve(); 

    cout << "[ApproveCommand] Approved work component.\n";
    return true;
}

void ApproveCommand::undo() {
    if (!item) return;

    item->restoreState(previousState);
    cout << "[AssignCommand] Undid approval, restored state silently to: " << previousState << "\n";
}