#include "ResourceCoordinator.h"
#include "WorkItem.h"
#include "Resource.h"
#include "Colleague.h"
#include <algorithm>

ResourceCoordinator::ResourceCoordinator()
{

}

ResourceCoordinator::~ResourceCoordinator()
{

}

bool ResourceCoordinator::canBecomeAvailable(WorkItem* item)
{
    if (item == nullptr)
    {
        return false;
    }

    std::map<WorkItem*, std::vector<WorkItem*> >::iterator it = dependencies.find(item);
    if (it == dependencies.end())
    {
        return true;
    }

    for (size_t i = 0; i < it->second.size(); ++i)
    {
        if (!it->second[i]->isComplete())//need to add isCompleted to workItem and states
        {
            return false;
        }
    }
    return true;
}

bool ResourceCoordinator::requestResource(WorkItem* item, Resource* resource)
{
    if (item == nullptr || resource == nullptr)
    {
        return false;
    }

    if (!resource->isFree())
    {
        return false;
    }

    allocations[resource].push_back(item);
    return true;
}

void ResourceCoordinator::release(WorkItem* item)
{
    if (item == nullptr)
    {
        return;
    }

    for (auto& pair : allocations)
    {
        std::vector<WorkItem*>& items = pair.second;
        items.erase(std::remove(items.begin(), items.end(), item), items.end());
    }
}

void ResourceCoordinator::notifyChanged(Colleague* colleague, const std::string& event)
{
    if (colleague == nullptr)
    {
        return;
    }

    std::map<WorkItem*, std::vector<WorkItem*> >::iterator it;
    for (it = dependencies.begin(); it != dependencies.end(); ++it)
    {
        for (size_t i = 0; i < it->second.size(); ++i)
        {
            Colleague* prerequisite = it->second[i];
            if (prerequisite == colleague)
            {
                it->first->receive(event, colleague);
                break;
            }
        }
    }
}

void ResourceCoordinator::registerColleague(Colleague* colleague)
{
    if (colleague == nullptr)
    {
        return;
    }

    // Duplication check
    if (std::find(colleagues.begin(), colleagues.end(), colleague) != colleagues.end())
    {
        return;
    }

    colleagues.push_back(colleague);
    colleague->setCoordinator(this);
}

void ResourceCoordinator::addDependency(WorkItem* item, WorkItem* prerequisite)
{
    if (item == nullptr || prerequisite == nullptr || item == prerequisite)
    {
        return;
    }

    std::vector<WorkItem*>& prerequisites = dependencies[item];

    if (std::find(prerequisites.begin(), prerequisites.end(), prerequisite) != prerequisites.end())
    {
        return;
    }

    prerequisites.push_back(prerequisite);
}