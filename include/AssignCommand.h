#ifndef ASSIGNCOMMAND_H
#define ASSIGNCOMMAND_H

#include <string>
#include "WorkflowCommand.h"

class WorkComponent;

using namespace std;

class AssignCommand : public WorkflowCommand {
private:
    WorkComponent* item;
    string newParticipant;
    string previousParticipant;

public:
    AssignCommand(WorkComponent* workComponent, const string& participant);

    bool execute() override;
    void undo() override;
};

#endif