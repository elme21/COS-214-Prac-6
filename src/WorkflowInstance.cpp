/**
 * @file WorkflowInstance.cpp
 * @brief Implementation of the WorkflowInstance originator class.
 */
#include "WorkflowInstance.h"
#include "WorkComponent.h" 

WorkflowInstance::WorkflowInstance(shared_ptr<WorkComponent> rootNode, const string& initialStatus)
    : root(rootNode), status(initialStatus) {}

shared_ptr<InstanceSnapshot> WorkflowInstance::createSnapshot() {
    // Deep clone the root component using the Prototype's clone() method
    shared_ptr<WorkComponent> clonedRoot = nullptr;
    if (root) {
        clonedRoot = shared_ptr<WorkComponent>(root->clone());
    }
    return make_shared<InstanceSnapshot>(status, clonedRoot);
}

void WorkflowInstance::restoreSnapshot(shared_ptr<InstanceSnapshot> snapshot) {
    if (snapshot) {
        status = snapshot->savedStatus; // Accessible via friend class
        if (snapshot->savedRoot) {
            root = shared_ptr<WorkComponent>(snapshot->savedRoot->clone());
        }
    }
}

string WorkflowInstance::getStatus() const {
    return status;
}

void WorkflowInstance::setStatus(const string& newStatus) {
    status = newStatus;
}

shared_ptr<WorkComponent> WorkflowInstance::getRoot() const {
    return root;
}

void WorkflowInstance::setRoot(shared_ptr<WorkComponent> newRoot) {
    root = newRoot;
}

const map<string, string>& WorkflowInstance::getData() const {
    return data;
}

void WorkflowInstance::setData(const map<string, string>& newData) {
    data = newData;
}
