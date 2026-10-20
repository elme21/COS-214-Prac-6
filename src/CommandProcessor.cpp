#include "CommandProcessor.h"
#include <iostream>

void CommandProcessor::run(unique_ptr<WorkflowCommand> command) {
    if (command) {
        // Only push to history if execution succeeds
        if (command->execute()) {
            history.push(move(command));
        } else {
            cout << "[CommandProcessor] Command execution was blocked or refused.\n";
        }
    }
}

void CommandProcessor::undo() {
    if (!history.empty()) {
        history.top()->undo();
        history.pop();
    } else {
        cout << "[CommandProcessor] No commands to undo.\n";
    }
}