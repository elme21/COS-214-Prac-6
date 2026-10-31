#ifndef REJECTCOMMAND_H
#define REJECTCOMMAND_H

/**
 * @file RejectCommand.h
 * @brief Header for the rejection command.
 */

#include "WorkflowCommand.h"
#include "WorkComponent.h" 
#include "Participant.h"


/**
 * @class RejectCommand
 * @brief Encapsulates reject operations on a WorkComponent.
 */
class RejectCommand : public WorkflowCommand {
private:
    WorkComponent* item;                       /**< Target component receiver. */
    const WorkItemState* previousState;        /**< State prior to execution. */
    Participant* previousParticipant;          /**< Assignee prior to execution. */

public:
    /**
     * @brief Constructs a RejectCommand.
     * @param workComponent Pointer to the target component.
     */
    RejectCommand(WorkComponent* workComponent);

    /**
     * @brief Executes rejection on the target component.
     * @return true if successful, false otherwise.
     */
    bool execute() override;

    /**
     * @brief Silently restores previous state and assignee.
     */
    void undo() override;
};

#endif 
