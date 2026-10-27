// TEMPORARY - stands in for AssignCommand, CompleteCommand, ApproveCommand, RejectCommand and
// CancelCommand (Member 5) until those compile against WorkComponent (they assign a
// const WorkItemState* to a string, call restoreState with one argument and call a
// setAssignee(string) that does not exist). Delete when that lands.
#ifndef WEB_WEBCOMMANDS_H
#define WEB_WEBCOMMANDS_H

#include "WorkflowCommand.h"

#include <functional>
#include <string>
#include <vector>

class WorkComponent;
class WorkItemState;
class Participant;

/**
 * A ConcreteCommand for the engine's WorkflowCommand interface, run by the
 * engine's CommandProcessor.
 *
 * execute(): saves (state, assignee) of every task in the instance through the
 * public getters, then runs one user action, which only calls real engine
 * methods. The action may ripple (routing rule cancels a branch, the Mediator
 * unlocks dependants), so the whole instance is saved, not just one item.
 *
 * undo(): puts every task back with WorkComponent::restoreState, the back door
 * the shared contract reserves for command undo, then runs an undo hook for
 * side effects outside the tree (case data, resource allocations).
 */
class WebCommand : public WorkflowCommand {
public:
    typedef std::function<bool()> Action;
    typedef std::function<void()> UndoHook;

    WebCommand(const std::string& label, WorkComponent* root, Action action, bool* result, UndoHook undoHook);

    bool execute() override;
    void undo() override;

    const std::string& getLabel() const { return label; }

private:
    struct Saved {
        WorkComponent* component;
        const WorkItemState* state;
        Participant* assignee;
    };

    std::string label;
    WorkComponent* root;
    Action action;
    bool* result;
    UndoHook undoHook;
    std::vector<Saved> saved;
};

#endif
