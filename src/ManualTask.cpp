#include "ManualTask.h"

#include <iostream>

ManualTask::ManualTask() {}

WorkComponent* ManualTask::clone() const { return new ManualTask(*this); }

bool ManualTask::perform() {
    std::cout << "[ManualTask] waiting for its assignee to finish" << std::endl;
    return false;
}
