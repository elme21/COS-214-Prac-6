#ifndef CANCELCOMMAND_H
#define CANCELCOMMAND_H

/**
 * @file CancelCommand.h
 * @brief Header for the cancellation command.
 */

#include "WorkflowCommand.h"

class WorkComponent;
class WorkItemState;
class Participant;

/**
 * @class CancelCommand
 * @brief Encapsulates cancel operations on a WorkComponent.
 */
class CancelCommand : public WorkflowCommand {
private:
    WorkComponent* item;                       /**< Target component receiver. */
    const WorkItemState* previousState;        /**< State prior to execution. */
    Participant* previousParticipant;          /**< Assignee prior to execution. */

public:
    /**
     * @brief Constructs a CancelCommand.
     * @param workComponent Pointer to the target component.
     */
    CancelCommand(WorkComponent* workComponent);

    /**
     * @brief Executes cancellation on the target component.
     * @return true if successful, false otherwise.
     */
    bool execute() override;

    /**
     * @brief Silently restores previous state and assignee.
     */
    void undo() override;
};

#endif 
