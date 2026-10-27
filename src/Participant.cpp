#include "Participant.h"

Participant::Participant(const std::string& name, WorkCoordinator* coordinator)
    : Colleague(coordinator), name(name), assignedCount(0)
{

}

const std::string& Participant::getName() const
{
    return name;
}

Participant::~Participant()
{

}

int Participant::workload() const
{
    return assignedCount;
}

void Participant::itemAssigned()
{
    ++assignedCount;
}

void Participant::itemReleased()
{
    if (assignedCount > 0)
    {
        --assignedCount;
    }
}