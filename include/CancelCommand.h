#ifndef CANCELCOMMAND_H
#define CANCELCOMMAND_H

#include "WorkflowCommand.h"

class WorkComponent;
class WorkItemState;
class Participant;

class CancelCommand : public WorkflowCommand {
private:
    WorkComponent* item;
    const WorkItemState* previousState;
    Participant* previousParticipant;

public:
    CancelCommand(WorkComponent* workComponent);
    bool execute() override;
    void undo() override;
};

#endif
