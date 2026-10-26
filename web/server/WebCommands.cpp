// TEMPORARY - stands in for AssignCommand, CompleteCommand, ApproveCommand, RejectCommand and
// CancelCommand (Member 5) until those compile against WorkComponent. Delete when that lands.
#include "WebCommands.h"

#include "WorkComponent.h"
#include "WorkIterator.h"

#include <memory>

WebCommand::WebCommand(const std::string& commandLabel, WorkComponent* instanceRoot, Action userAction,
                       bool* outcome, UndoHook hook)
    : label(commandLabel), root(instanceRoot), action(userAction), result(outcome), undoHook(hook) {}

bool WebCommand::execute() {
    saved.clear();
    std::unique_ptr<WorkIterator> it(root->createIterator());
    it->reset();
    while (it->hasNext()) {
        WorkComponent* node = it->next();
        if (node->getState() == nullptr) continue;   // stages have no state
        Saved entry = { node, node->getState(), node->getAssignee() };
        saved.push_back(entry);
    }

    bool ok = action ? action() : false;
    action = Action();   // the action refers to the request that created it; never run it twice
    if (result != nullptr) *result = ok;
    return ok;
}

void WebCommand::undo() {
    for (size_t i = 0; i < saved.size(); ++i) {
        saved[i].component->restoreState(saved[i].state, saved[i].assignee);
    }
    if (undoHook) undoHook();
}
