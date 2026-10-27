#ifndef APPROVECOMMAND_H
#define APPROVECOMMAND_H

/**
 * @file ApproveCommand.h
 * @brief Header for the approval command.
 */

#include "WorkflowCommand.h"

/**
 * @class ApproveCommand
 * @brief Encapsulates approve operations on a WorkComponent.
 */
class ApproveCommand : public WorkflowCommand {
private:
    WorkComponent* item;                       /**< Target component receiver. */
    const WorkItemState* previousState;        /**< State prior to execution. */
    Participant* previousParticipant;          /**< Assignee prior to execution. */

public:
    /**
     * @brief Constructs an ApproveCommand.
     * @param workComponent Pointer to the target component.
     */
    ApproveCommand(WorkComponent* workComponent);

    /**
     * @brief Executes approval on the target component.
     * @return true if successful, false otherwise.
     */
    bool execute() override;

    /**
     * @brief Silently restores previous state and assignee.
     */
    void undo() override;
};
#endif
