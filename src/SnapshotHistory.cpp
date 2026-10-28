/**
 * @file SnapshotHistory.cpp
 * @brief Implementation of the SnapshotHistory caretaker class.
 */
#include "SnapshotHistory.h"

void SnapshotHistory::save(shared_ptr<InstanceSnapshot> snapshot) {
    if (snapshot) {
        snapshots.push_back(snapshot);
    }
}

shared_ptr<InstanceSnapshot> SnapshotHistory::latest() {
    if (snapshots.empty()) {
        return nullptr;
    }
    return snapshots.back();
}

shared_ptr<InstanceSnapshot> SnapshotHistory::pop() {
    if (snapshots.empty()) {
        return nullptr;
    }
    auto snapshot = snapshots.back();
    snapshots.pop_back();
    return snapshot;
}
