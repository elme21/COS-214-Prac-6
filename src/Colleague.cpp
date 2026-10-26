#include "Colleague.h"
#include "WorkCoordinator.h"

Colleague::Colleague(WorkCoordinator* coordinator) : coordinator(coordinator)
{

}

Colleague::~Colleague()
{

}

void Colleague::setCoordinator(WorkCoordinator* coordinator)
{
    this->coordinator = coordinator;
}

void Colleague::changed(const std::string& event)
{
    if (coordinator != nullptr)
    {
        coordinator->notifyChanged(this, event);
    }
}

void Colleague::receive(const std::string& event, Colleague* source)
{
    // Default implementation does nothing and gets overridden
}