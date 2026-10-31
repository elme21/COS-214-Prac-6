/**
 * @file InstanceSnapshot.cpp
 * @brief Implementation of the InstanceSnapshot class.
 */
#include "InstanceSnapshot.h"

InstanceSnapshot::InstanceSnapshot(const string& status, shared_ptr<WorkComponent> rootComponent, const map<string, string>& data)
    : savedStatus(status), savedRoot(rootComponent), savedData(data), takenAtTime(chrono::system_clock::now()) {}

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

const map<string, string>& InstanceSnapshot::getSavedData() const {
    return savedData;
}
