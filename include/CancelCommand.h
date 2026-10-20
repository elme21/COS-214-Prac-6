#ifndef CANCELCOMMAND_H
#define CANCELCOMMAND_H

#include <string>
#include "WorkflowCommand.h"

class WorkComponent;

using namespace std;

class CancelCommand : public WorkflowCommand {
private:
    WorkComponent* item;
    string previousState;

public:
    CancelCommand(WorkComponent* workComponent);

    bool execute() override;
    void undo() override;
};

#endif