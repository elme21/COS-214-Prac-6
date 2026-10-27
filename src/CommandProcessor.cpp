#include "CommandProcessor.h"

bool CommandProcessor::run(unique_ptr<WorkflowCommand> command) {
    if (command && command->execute()) {
        history.push(move(command));
        return true; // Execution succeeded and pushed to history
    }
    return false; // Blocked by decorators/validators, not pushed
}

void CommandProcessor::undo() {
    if (!history.empty()) {
        history.top()->undo();
        history.pop();
    }
}

const stack<unique_ptr<WorkflowCommand>>& CommandProcessor::getHistory() const {
    return history;
}
