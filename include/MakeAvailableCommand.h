#ifndef MAKEAVAILABLECOMMAND_H
#define MAKEAVAILABLECOMMAND_H

#include "WorkflowCommand.h"

class WorkComponent;
class WorkItemState;
class Participant;

class MakeAvailableCommand : public WorkflowCommand {
private:
    WorkComponent* item;
    const WorkItemState* previousState;
    Participant* previousParticipant;

public:
    MakeAvailableCommand(WorkComponent* workComponent);
    bool execute() override;
    void undo() override;
};

#endif
