#ifndef ASSIGNCOMMAND_H
#define ASSIGNCOMMAND_H

/**
 * @file AssignCommand.h
 * @brief Header for the assignment command.
 */

#include "WorkflowCommand.h"
#include "WorkComponent.h" 
#include "Participant.h"

class WorkComponent;
class WorkItemState;
class Participant;

using namespace std;

/**
 * @class AssignCommand
 * @brief Encapsulates assignTo operations on a WorkComponent.
 */
class AssignCommand : public WorkflowCommand {
private:
    WorkComponent* item;                       /**< Target component receiver. */
    Participant* newParticipant;               /**< Participant being assigned. */
    Participant* previousParticipant;          /**< Assignee prior to execution. */
    const WorkItemState* previousState;        /**< State prior to execution. */

public:
    /**
     * @brief Constructs an AssignCommand.
     * 
     * @param workComponent Pointer to the target component.
     * @param participant Pointer to the new assignee.
     */
    AssignCommand(WorkComponent* workComponent, Participant* participant);

    /**
     * @brief Executes assignment to the target participant.
     * @return true if successful, false otherwise.
     */
    bool execute() override;

    /**
     * @brief Silently restores previous assignee and state.
     */
    void undo() override;
};

#endif
