#ifndef COMPLETECOMMAND_H
#define COMPLETECOMMAND_H

#include "WorkflowCommand.h"

class WorkComponent;
class WorkItemState;
class Participant;

class CompleteCommand : public WorkflowCommand {
private:
    WorkComponent* item;
    const WorkItemState* previousState;
    Participant* previousParticipant;

public:
    CompleteCommand(WorkComponent* workComponent);
    bool execute() override;
    void undo() override;
};

#endif
