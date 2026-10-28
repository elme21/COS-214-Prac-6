#include "AutomatedTask.h"

#include <iostream>

AutomatedTask::AutomatedTask() : stepSucceeds(true) {}

AutomatedTask::AutomatedTask(const std::string& name, const std::string& id)
    : WorkItem(name, id), stepSucceeds(true) {}

void AutomatedTask::setStepSucceeds(bool succeeds) { stepSucceeds = succeeds; }

bool AutomatedTask::getStepSucceeds() const { return stepSucceeds; }

WorkComponent* AutomatedTask::clone() const { return new AutomatedTask(*this); }

bool AutomatedTask::perform() {
    std::cout << "[AutomatedTask] running automated step '" << getName() << "': "
              << (stepSucceeds ? "succeeded" : "failed") << std::endl;
    return stepSucceeds;
}
