#include "Participant.h"

Participant::Participant(WorkCoordinator* coordinator) : Colleague(coordinator), assignedCount(0)
{

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