#ifndef SNAPSHOTHISTORY_H
#define SNAPSHOTHISTORY_H

/**
 * @file SnapshotHistory.h
 * @brief Header for the SnapshotHistory class (Caretaker in Memento Pattern).
 */

#include <vector>
#include <memory>
#include "InstanceSnapshot.h"

using namespace std;

/**
 * @class SnapshotHistory
 * @brief Caretaker class that manages a chronological stack of InstanceSnapshot objects.
 * 
 * Provides operations to record new snapshots, inspect the most recent checkpoint,
 * and pop snapshots for restoration.
 */
class SnapshotHistory {
private:
    vector<shared_ptr<InstanceSnapshot>> snapshots; /**< Chronological collection of saved snapshots. */

public:
    /**
     * @brief Saves a snapshot to the history sequence.
     * @param snapshot Shared pointer to the snapshot to store.
     */
    void save(shared_ptr<InstanceSnapshot> snapshot);

    /**
     * @brief Returns the most recent snapshot without removing it.
     * @return Shared pointer to the latest snapshot, or nullptr if history is empty.
     */
    shared_ptr<InstanceSnapshot> latest();

    /**
     * @brief Pops and returns the most recent snapshot from history.
     * @return Shared pointer to the popped snapshot, or nullptr if history is empty.
     */
    shared_ptr<InstanceSnapshot> pop();
};

#endif
