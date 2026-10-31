#ifndef MAKEAVAILABLECOMMAND_H
#define MAKEAVAILABLECOMMAND_H

/**
 * @file MakeAvailableCommand.h
 * @brief Header for the make-available command.
 */

#include "WorkflowCommand.h"
#include "WorkComponent.h" 
#include "Participant.h"

/**
 * @class MakeAvailableCommand
 * @brief Encapsulates makeAvailable operations on a WorkComponent.
 */
class MakeAvailableCommand : public WorkflowCommand {
private:
    WorkComponent* item;                       /**< Target component receiver. */
    const WorkItemState* previousState;        /**< State prior to execution. */
    Participant* previousParticipant;          /**< Assignee prior to execution. */

public:
    /**
     * @brief Constructs a MakeAvailableCommand.
     * @param workComponent Pointer to the target component.
     */
    MakeAvailableCommand(WorkComponent* workComponent);

    /**
     * @brief Executes makeAvailable on the target component.
     * @return true if successful, false otherwise.
     */
    bool execute() override;

    /**
     * @brief Silently restores previous state and assignee.
     */
    void undo() override;
};

#endif 
