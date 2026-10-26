#ifndef RESOURCECOORDINATOR_H
#define RESOURCECOORDINATOR_H

#include <map>
#include <vector>
#include <string>
#include "WorkCoordinator.h"

class Colleague;
class WorkItem;
class Resource;

class ResourceCoordinator : public WorkCoordinator
{
    public:
        ResourceCoordinator();
        virtual ~ResourceCoordinator();

        bool canBecomeAvailable(WorkItem* item) override;
        bool requestResource(WorkItem* item, Resource* resource) override;
        void release(WorkItem* item) override;
        void notifyChanged(Colleague* colleague, const std::string& event) override;

        void registerColleague(Colleague* colleague);
        void addDependency(WorkItem* item, WorkItem* prerequisite);

    private:
        std::map<WorkItem*, std::vector<WorkItem*> > dependencies;
        std::map<Resource*, std::vector<WorkItem*> > allocations;
        std::vector<Colleague*> colleagues;
};

#endif