#ifndef WORKFLOWCOMMAND_H
#define WORKFLOWCOMMAND_H

/**
 * @file WorkflowCommand.h
 * @brief Base interface for all workflow commands (Command Pattern).
 */

/**
 * @class WorkflowCommand
 * @brief Abstract base class representing an executable workflow command.
 * 
 * Defines the core contract for executing state transitions on a target WorkComponent
 * and silently rolling them back on undo.
 */
class WorkflowCommand {
public:
    /**
     * @brief Virtual destructor for proper polymorphic cleanup.
     */
    virtual ~WorkflowCommand() = default;

    /**
     * @brief Executes the command action on the receiver component.
     * 
     * @return true if execution succeeded and was unblocked by decorators, false otherwise.
     */
    virtual bool execute() = 0;

    /**
     * @brief Silently rolls back the receiver component to its prior state.
     */
    virtual void undo() = 0;
};

#endif
