#ifndef WORKCOORDINATOR_H
#define WORKCOORDINATOR_H

#include <string>

class Colleague;
class WorkItem;
class Resource;

class WorkCoordinator
{
    public:
        virtual ~WorkCoordinator() {}
        virtual bool canBecomeAvailable(WorkItem* item) = 0;
        virtual bool requestResource(WorkItem* item, Resource* resource) = 0;
        virtual void release(WorkItem* item) = 0;
        virtual void notifyChanged(Colleague* colleague, const std::string& event) = 0;
};

#endif