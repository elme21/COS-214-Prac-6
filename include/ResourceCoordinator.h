#ifndef RESOURCECOORDINATOR_H
#define RESOURCECOORDINATOR_H

#include <map>
#include <vector>
#include <string>
#include "WorkCoordinator.h"

class Colleague;
class WorkItem;
class Resource;
/// @brief 
/// The ResourceCoordinator class is a ConcreteMediator that manages the allocation of resources to work items. It maintains dependencies between work items and tracks resource allocations.
class ResourceCoordinator : public WorkCoordinator
{
    public:
        /// @brief The constructor
        ResourceCoordinator();
        /// @brief The destructor
        virtual ~ResourceCoordinator();

        /// @brief checks if the work item can become available based on its dependencies
        /// @param item The work item to check for availability
        /// @return true if the work item can become available, false otherwise
        bool canBecomeAvailable(WorkItem* item) override;
        /// @brief requests a resource for a work item to use. If the resource is available, it is allocated to the work item.
        /// @param item the work item requesting the resource
        /// @param resource the resource being requested
        /// @return true if the resource is allocated, false otherwise
        bool requestResource(WorkItem* item, Resource* resource) override;
        /// @brief releases a resource from a work item
        /// @param item the work item releasing the resource
        void release(WorkItem* item) override;
        /// @brief notifies the coordinator that a colleague has changed
        /// @param colleague the colleague that has changed
        /// @param event the event that occurred
        void notifyChanged(Colleague* colleague, const std::string& event) override;

        /// @brief registers a colleague with the coordinator
        /// @param colleague the colleague to register
        void registerColleague(Colleague* colleague);
        /// @brief adds a dependency between two work items
        /// @param item the work item that depends on the prerequisite
        /// @param prerequisite the work item that must be completed before the item can be started
        void addDependency(WorkItem* item, WorkItem* prerequisite);

    private:
        /// @brief A map that tracks the dependencies between work items. Each work item maps to a vector of its prerequisites.
        std::map<WorkItem*, std::vector<WorkItem*> > dependencies;
        /// @brief A map that tracks the allocation of resources to work items.
        std::map<Resource*, std::vector<WorkItem*> > allocations;
        /// @brief A vector that holds pointers to all registered colleagues.
        std::vector<Colleague*> colleagues;
};

#endif