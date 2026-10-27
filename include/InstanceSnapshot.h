#ifndef INSTANCESNAPSHOT_H
#define INSTANCESNAPSHOT_H

/**
 * @file InstanceSnapshot.h
 * @brief Header for the InstanceSnapshot class (Memento Pattern).
 */
#include <string>
#include <chrono>
#include <memory>

class WorkComponent;
class WorkflowInstance;

using namespace std;

/**
 * @class InstanceSnapshot
 * @brief Immutable snapshot (Memento) capturing the state and component hierarchy of a WorkflowInstance.
 * 
 * Stores the workflow status, a cloned root component tree, and the timestamp when the snapshot was taken.
 * State restoration logic is restricted to WorkflowInstance via friend declaration.
 */
class InstanceSnapshot {
private:
    string savedStatus;                             /**< Preserved workflow status string. */
    shared_ptr<WorkComponent> savedRoot;           /**< Deep-cloned root component tree state. */
    chrono::system_clock::time_point takenAtTime;   /**< Timestamp when snapshot was captured. */

    friend class WorkflowInstance;

    /**
     * @brief Internal helper to retrieve saved status.
     * @return Saved status string.
     */
    string getState() const;

public:
    /**
     * @brief Constructs an InstanceSnapshot with a given status and cloned root component.
     * 
     * @param status Current status of the workflow instance.
     * @param rootComponent Deep-cloned root WorkComponent tree.
     */
    InstanceSnapshot(const string& status, shared_ptr<WorkComponent> rootComponent);

    /**
     * @brief Returns a human-readable label for the snapshot.
     * @return Descriptive label including the status at snapshot creation.
     */
    string label() const;

    /**
     * @brief Retrieves the time point when the snapshot was captured.
     * @return System clock time point.
     */
    chrono::system_clock::time_point takenAt() const;

    /**
     * @brief Accesses the cloned root component tree stored within the snapshot.
     * @return Shared pointer to the saved root WorkComponent.
     */
    shared_ptr<WorkComponent> getSavedRoot() const;
};

#endif
