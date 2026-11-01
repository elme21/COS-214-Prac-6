#ifndef COMPLETECOMMAND_H
#define COMPLETECOMMAND_H

/**
 * @file CompleteCommand.h
 * @brief Header for the completion command.
 */

#include "WorkflowCommand.h"
#include "WorkComponent.h" 
#include "Participant.h"


/**
 * @class CompleteCommand
 * @brief Encapsulates complete operations on a WorkComponent.
 */
class CompleteCommand : public WorkflowCommand {
private:
    WorkComponent* item;                       /**< Target component receiver. */
    const WorkItemState* previousState;        /**< State prior to execution. */
    Participant* previousParticipant;          /**< Assignee prior to execution. */

public:
    /**
     * @brief Constructs a CompleteCommand.
     * @param workComponent Pointer to the target component.
     */
    CompleteCommand(WorkComponent* workComponent);

    /**
     * @brief Executes completion on the target component.
     * @return true if successful, false otherwise.
     */
    bool execute() override;

    /**
     * @brief Silently restores previous state and assignee.
     */
    void undo() override;
};

#endif
