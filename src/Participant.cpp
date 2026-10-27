#include "Participant.h"

Participant::Participant(const std::string& id, const std::string& name, const std::string& role, WorkCoordinator* coordinator)
    : Colleague(coordinator), id(id), name(name), role(role), assignedCount(0)
{

}

const std::string& Participant::getID() const
{
    return id;
}

const std::string& Participant::getName() const
{
    return name;
}

const std::string& Participant::getRole() const
{
    return role;
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