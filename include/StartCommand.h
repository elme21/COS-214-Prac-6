#ifndef STARTCOMMAND_H
#define STARTCOMMAND_H

#include "WorkflowCommand.h"

class WorkComponent;
class WorkItemState;
class Participant;

class StartCommand : public WorkflowCommand {
private:
    WorkComponent* item;
    const WorkItemState* previousState;
    Participant* previousParticipant;

public:
    StartCommand(WorkComponent* workComponent);
    bool execute() override;
    void undo() override;
};

#endif
