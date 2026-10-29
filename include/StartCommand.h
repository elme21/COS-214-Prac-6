#ifndef STARTCOMMAND_H
#define STARTCOMMAND_H

/**
 * @file StartCommand.h
 * @brief Header for the start command.
 */

#include "WorkflowCommand.h"
#include "WorkComponent.h" 
#include "Participant.h"

class WorkComponent;
class WorkItemState;
class Participant;

/**
 * @class StartCommand
 * @brief Encapsulates start lifecycle operations on a WorkComponent.
 */
class StartCommand : public WorkflowCommand {
private:
    WorkComponent* item;                       /**< Target component receiver. */
    const WorkItemState* previousState;        /**< State prior to execution. */
    Participant* previousParticipant;          /**< Assignee prior to execution. */

public:
    /**
     * @brief Constructs a StartCommand.
     * @param workComponent Pointer to the target component.
     */
    StartCommand(WorkComponent* workComponent);

    /**
     * @brief Executes start on the target component.
     * @return true if successful, false otherwise.
     */
    bool execute() override;

    /**
     * @brief Silently restores previous state and assignee.
     */
    void undo() override;
};

#endif 
