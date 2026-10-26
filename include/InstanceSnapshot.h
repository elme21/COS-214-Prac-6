#ifndef INSTANCESNAPSHOT_H
#define INSTANCESNAPSHOT_H

#include <string>
#include <chrono>
#include <memory>

class WorkComponent;
class WorkflowInstance;

using namespace std;

class InstanceSnapshot {
private:
    string savedStatus;
    shared_ptr<WorkComponent> savedRoot;
    chrono::system_clock::time_point takenAtTime;

    friend class WorkflowInstance;

    string getState() const;

public:
    InstanceSnapshot(const string& status, shared_ptr<WorkComponent> rootComponent);

    string label() const;
    chrono::system_clock::time_point takenAt() const;
    shared_ptr<WorkComponent> getSavedRoot() const;
};

#endif