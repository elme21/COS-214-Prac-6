#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include <string>
#include <memory>
#include <map>
#include "InstanceSnapshot.h"

class WorkComponent;

using namespace std;

class WorkflowInstance {
private:
    shared_ptr<WorkComponent> root;
    string status;
    map<string, string> data;

public:
    WorkflowInstance(shared_ptr<WorkComponent> rootNode, const string& initialStatus);

    shared_ptr<InstanceSnapshot> createSnapshot();
    void restoreSnapshot(shared_ptr<InstanceSnapshot> snapshot);
    
    string getStatus() const;
    void setStatus(const string& newStatus);
    shared_ptr<WorkComponent> getRoot() const;
    void setRoot(shared_ptr<WorkComponent> newRoot);
};

#endif
