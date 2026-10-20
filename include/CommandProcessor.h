#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

#include <stack>
#include <memory>
#include "WorkflowCommand.h"

using namespace std;

class CommandProcessor {
private:
    stack<unique_ptr<WorkflowCommand>> history;

public:
    void run(unique_ptr<WorkflowCommand> command);
    void undo();
};

#endif