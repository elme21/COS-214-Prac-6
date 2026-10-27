#ifndef ASSIGNCOMMAND_H
#define ASSIGNCOMMAND_H

#include "WorkflowCommand.h"

class WorkComponent;
class WorkItemState;
class Participant;

using namespace std;

class AssignCommand : public WorkflowCommand {
private:
    WorkComponent* item;
    Participant* newParticipant;
    Participant* previousParticipant;
    const WorkItemState* previousState;

public:
    AssignCommand(WorkComponent* workComponent, Participant* participant);

    bool execute() override;
    void undo() override;
};

#endif
