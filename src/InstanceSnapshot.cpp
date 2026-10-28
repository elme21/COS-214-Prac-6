/**
 * @file InstanceSnapshot.cpp
 * @brief Implementation of the InstanceSnapshot class.
 */
#include "InstanceSnapshot.h"

InstanceSnapshot::InstanceSnapshot(const string& status, shared_ptr<WorkComponent> rootComponent)
    : savedStatus(status), savedRoot(rootComponent), takenAtTime(chrono::system_clock::now()) {}

string InstanceSnapshot::getState() const {
    return savedStatus;
}

string InstanceSnapshot::label() const {
    return "Snapshot taken at status: " + savedStatus;
}

chrono::system_clock::time_point InstanceSnapshot::takenAt() const {
    return takenAtTime;
}

shared_ptr<WorkComponent> InstanceSnapshot::getSavedRoot() const {
    return savedRoot;
}
