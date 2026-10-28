#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

/**
 * @file CommandProcessor.h
 * @brief Header for the command invoker and history manager.
 */

#include <stack>
#include <memory>
#include "WorkflowCommand.h"

using namespace std;

/**
 * @class CommandProcessor
 * @brief Invoker class responsible for executing commands and maintaining undo history.
 * 
 * Evaluates decorator/validator execution guards before pushing commands onto the history stack.
 */
class CommandProcessor {
private:
    stack<unique_ptr<WorkflowCommand>> history; /**< Stack of executed commands for undo operation. */

public:
    /**
     * @brief Executes a given workflow command.
     * 
     * @param command A unique pointer to the workflow command to execute.
     * @return true if execution succeeded and was pushed to history, false if blocked by validation.
     */
    bool run(unique_ptr<WorkflowCommand> command);

    /**
     * @brief Rolls back the most recent workflow command in history.
     */
    void undo();

    /**
     * @brief Retrieves the stack of executed commands for UI inspection.
     * 
     * @return Const reference to the underlying command history stack.
     */
    const stack<unique_ptr<WorkflowCommand>>& getHistory() const;
};

#endif
