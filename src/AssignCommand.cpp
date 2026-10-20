#include "AssignCommand.h"
#include "WorkComponent.h" // Assumed to have getAssignee() and setAssignee()
#include <iostream>

AssignCommand::AssignCommand(WorkComponent* workComponent, const string& participant)
    : item(workComponent), newParticipant(participant), previousParticipant("") {}

bool AssignCommand::execute() {
    if (!item) return false;
    
    // Store previous participant for undo
    previousParticipant = item->getAssignee(); 
    item->setAssignee(newParticipant);
    
    cout << "[AssignCommand] Assigned component to " << newParticipant << "\n";
    return true;
}

void AssignCommand::undo() {
    if (!item) return;
    
    // Restore previous participant silently
    item->setAssignee(previousParticipant);
    cout << "[AssignCommand] Reverted assignment back to " << previousParticipant << "\n";
}