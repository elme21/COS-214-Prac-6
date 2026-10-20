#ifndef APPROVECOMMAND_H
#define APPROVECOMMAND_H

#include <string>
#include "WorkflowCommand.h"

class WorkComponent;

using namespace std;

class ApproveCommand : public WorkflowCommand {
private:
    WorkComponent* item;
    string previousState;

public:
    ApproveCommand(WorkComponent* workComponent);

    bool execute() override;
    void undo() override;
};

#endif