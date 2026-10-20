#ifndef WORKFLOWCOMMAND_H
#define WORKFLOWCOMMAND_H

class WorkflowCommand {
public:
    virtual ~WorkflowCommand() = default;
    virtual bool execute() = 0;
    virtual void undo() = 0;
};

#endif