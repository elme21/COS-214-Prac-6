#ifndef WORKCOORDINATOR_H
#define WORKCOORDINATOR_H

#include <string>

class Colleague;
class WorkItem;
class Resource;

/// @brief 
/// WorkCoordinator is the Mediator
class WorkCoordinator
{
    public:
        /// @brief the default destructor
        virtual ~WorkCoordinator() {}
        /// @brief checks if the work item can become available based on its dependencies. (implemented by its children classes)
        /// @param item the work item to check for availability
        /// @return true if the work item can become available, false otherwise
        virtual bool canBecomeAvailable(WorkItem* item) = 0;
        /// @brief requests a resource for a work item to use. (implemented by its children classes)
        /// @param item the work item requesting the resource
        /// @param resource the resource being requested
        /// @return true if the resource is allocated, false otherwise
        virtual bool requestResource(WorkItem* item, Resource* resource) = 0;
        /// @brief releases a resource from a work item. (implemented by its children classes)
        /// @param item the work item releasing the resource
        virtual void release(WorkItem* item) = 0;
        /// @brief notifies the coordinator that a colleague has changed. (implemented by its children classes)
        /// @param colleague the colleague that has changed
        /// @param event the event that occurred
        virtual void notifyChanged(Colleague* colleague, const std::string& event) = 0;
};

#endif