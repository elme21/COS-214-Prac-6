#ifndef REJECTCOMMAND_H
#define REJECTCOMMAND_H

#include <string>
#include "WorkflowCommand.h"

class WorkComponent;

using namespace std;

class RejectCommand : public WorkflowCommand {
private:
    WorkComponent* item;
    string previousState;

public:
    RejectCommand(WorkComponent* workComponent);

    bool execute() override;
    void undo() override;
};

#endif