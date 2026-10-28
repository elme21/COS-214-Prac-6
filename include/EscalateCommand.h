#ifndef ESCALATECOMMAND_H
#define ESCALATECOMMAND_H

/**
 * @file EscalateCommand.h
 * @brief Header for the escalation command.
 */

#include "WorkflowCommand.h"

class WorkComponent;
class WorkItemState;
class Participant;

/**
 * @class EscalateCommand
 * @brief Encapsulates escalate operations on a WorkComponent.
 */
class EscalateCommand : public WorkflowCommand {
private:
    WorkComponent* item;                       /**< Target component receiver. */
    const WorkItemState* previousState;        /**< State prior to execution. */
    Participant* previousParticipant;          /**< Assignee prior to execution. */

public:
    /**
     * @brief Constructs an EscalateCommand.
     * @param workComponent Pointer to the target component.
     */
    EscalateCommand(WorkComponent* workComponent);

    /**
     * @brief Executes escalation on the target component.
     * @return true if successful, false otherwise.
     */
    bool execute() override;

    /**
     * @brief Silently restores previous state and assignee.
     */
    void undo() override;
};

#endif 
