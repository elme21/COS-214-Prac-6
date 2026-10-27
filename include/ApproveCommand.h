#ifndef APPROVECOMMAND_H
#define APPROVECOMMAND_H

#include "WorkflowCommand.h"

class WorkComponent;
class WorkItemState;
class Participant;

class ApproveCommand : public WorkflowCommand {
private:
    WorkComponent* item;
    const WorkItemState* previousState;
    Participant* previousParticipant;

public:
    ApproveCommand(WorkComponent* workComponent);
    bool execute() override;
    void undo() override;
};

#endif
