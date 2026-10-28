/**
 * @file AssignCommand.cpp
 * @brief Implementation of AssignCommand.
 */
#include "AssignCommand.h"
#include "WorkComponent.h"

AssignCommand::AssignCommand(WorkComponent* workComponent, Participant* participant)
    : item(workComponent), newParticipant(participant), previousParticipant(nullptr), previousState(nullptr) {}

bool AssignCommand::execute() {
    if (!item) return false;
    
    //Use const WorkItemState* and getAssignee()
    previousState = item->getState();
    previousParticipant = item->getAssignee();
    
    return item->assignTo(newParticipant);
}

void AssignCommand::undo() {
    if (item) {
        //Use combined restoreState signature
        item->restoreState(previousState, previousParticipant);
    }
}
