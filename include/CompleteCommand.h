#ifndef COMPLETECOMMAND_H
#define COMPLETECOMMAND_H

#include <string>
#include "WorkflowCommand.h"

class WorkComponent;

using namespace std;

class CompleteCommand : public WorkflowCommand {
private:
    WorkComponent* item;
    string previousState;

public:
    CompleteCommand(WorkComponent* workComponent);

    bool execute() override;
    void undo() override;
};

#endif