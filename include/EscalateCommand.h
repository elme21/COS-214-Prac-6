#ifndef ESCALATECOMMAND_H
#define ESCALATECOMMAND_H

#include "WorkflowCommand.h"

class WorkComponent;
class WorkItemState;
class Participant;

class EscalateCommand : public WorkflowCommand {
private:
    WorkComponent* item;
    const WorkItemState* previousState;
    Participant* previousParticipant;

public:
    EscalateCommand(WorkComponent* workComponent);
    bool execute() override;
    void undo() override;
};

#endif
