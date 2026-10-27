#include "Resource.h"

Resource::Resource(const std::string& name, int capacity, WorkCoordinator* coordinator)
    : Colleague(coordinator), name(name), capacity(capacity), available(capacity)
{

}

Resource::~Resource()
{

}

const std::string& Resource::getName() const
{
    return name;
}

int Resource::getCapacity() const
{
    return capacity;
}

int Resource::getAvailable() const
{ 
    return available; 
}

bool Resource::isFree() const
{ 
    return available > 0; 
}

void Resource::take()
{
    if (available > 0)
    {
        --available;
    }
}

void Resource::giveBack()
{
    if (available < capacity)
    {
        ++available;
    }
}