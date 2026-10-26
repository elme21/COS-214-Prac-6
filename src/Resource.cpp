#include "Resource.h"

Resource::Resource(int capacity, WorkCoordinator* coordinator) : Colleague(coordinator), capacity(capacity)
{

}

Resource::~Resource()
{

}

bool Resource::isFree() const
{
    return capacity > 0; //needs changing for an upperlimit but for now this will surve as a placeholder
}