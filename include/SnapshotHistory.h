#ifndef SNAPSHOTHISTORY_H
#define SNAPSHOTHISTORY_H

#include <vector>
#include <memory>
#include "InstanceSnapshot.h"

using namespace std;

class SnapshotHistory {
private:
    vector<shared_ptr<InstanceSnapshot>> snapshots;

public:
    void save(shared_ptr<InstanceSnapshot> snapshot);
    shared_ptr<InstanceSnapshot> latest();
    shared_ptr<InstanceSnapshot> pop();
};

#endif