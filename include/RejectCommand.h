#ifndef REJECTCOMMAND_H
#define REJECTCOMMAND_H

#include "WorkflowCommand.h"

class WorkComponent;
class WorkItemState;
class Participant;

class RejectCommand : public WorkflowCommand {
private:
    WorkComponent* item;
    const WorkItemState* previousState;
    Participant* previousParticipant;

public:
    RejectCommand(WorkComponent* workComponent);
    bool execute() override;
    void undo() override;
};

#endif
