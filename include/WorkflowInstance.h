#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

/**
 * @file WorkflowInstance.h
 * @brief Header for the WorkflowInstance class (Originator in Memento Pattern).
 */

#include <string>
#include <memory>
#include <map>
#include "InstanceSnapshot.h"

class WorkComponent;

using namespace std;

/**
 * @class WorkflowInstance
 * @brief Originator class representing an active, independent workflow execution.
 * 
 * Manages the component tree root, instance status, and key-value payload data.
 * Creates and restores immutable InstanceSnapshot mementos using Prototype deep cloning.
 */
class WorkflowInstance {
private:
    shared_ptr<WorkComponent> root; /**< Root of the component tree for this instance. */
    string status;                   /**< Current execution status of the workflow instance. */
    map<string, string> data;        /**< Instance payload data storage map. */

public:
    /**
     * @brief Constructs a WorkflowInstance.
     * 
     * @param rootNode Root component node of the workflow tree.
     * @param initialStatus Initial status string.
     */
    WorkflowInstance(shared_ptr<WorkComponent> rootNode, const string& initialStatus);

    /**
     * @brief Creates a snapshot of the current instance state (Memento).
     * 
     * Deep-clones the root component tree using Prototype clone() to guarantee instance isolation.
     * 
     * @return Shared pointer to the newly generated InstanceSnapshot.
     */
    shared_ptr<InstanceSnapshot> createSnapshot();

    /**
     * @brief Restores instance state from a snapshot.
     * 
     * Restores status and deep-clones saved root tree back into the active instance.
     * 
     * @param snapshot Pointer to the snapshot to restore from.
     */
    void restoreSnapshot(shared_ptr<InstanceSnapshot> snapshot);

    /**
     * @brief Retrieves the current status string.
     * @return Workflow status string.
     */
    string getStatus() const;

    /**
     * @brief Updates current workflow status.
     * @param newStatus New status string.
     */
    void setStatus(const string& newStatus);

    /**
     * @brief Gets the root WorkComponent of the active workflow tree.
     * @return Shared pointer to root WorkComponent.
     */
    shared_ptr<WorkComponent> getRoot() const;

    /**
     * @brief Sets a new root WorkComponent tree for the instance.
     * @param newRoot Shared pointer to the replacement root node.
     */
    void setRoot(shared_ptr<WorkComponent> newRoot);

    /**
     * @brief Retrieves the instance key-value data map.
     * @return Const reference to the internal data map.
     */
    const map<string, string>& getData() const;

    /**
     * @brief Sets or replaces the instance data payload map.
     * @param newData Map of key-value data to assign to the instance.
     */
    void setData(const map<string, string>& newData);
};

#endif
