// TEMPORARY - stands in for a name and role on Participant (Member 4) until Participant
// carries its own identity (getName()/getRole()). Delete when that lands.
#include "ParticipantDirectory.h"

#include "Participant.h"
#include "Resource.h"
#include "ResourceCoordinator.h"

ParticipantDirectory::ParticipantDirectory(ResourceCoordinator* mediator) : coordinator(mediator) {}

ParticipantDirectory::~ParticipantDirectory() {}

ParticipantEntry& ParticipantDirectory::add(const std::string& id, const std::string& name,
                                            const std::string& role, int slotCapacity) {
    std::unique_ptr<ParticipantEntry> entry(new ParticipantEntry());
    entry->id = id;
    entry->name = name;
    entry->role = role;
    entry->slotCapacity = slotCapacity;
    entry->participant.reset(new Participant(coordinator));
    coordinator->registerColleague(entry->participant.get());
    if (slotCapacity >= 0) {
        entry->slots.reset(new Resource(slotCapacity, coordinator));
        coordinator->registerColleague(entry->slots.get());
    }
    entries.push_back(std::move(entry));
    return *entries.back();
}

ParticipantEntry* ParticipantDirectory::find(const std::string& id) {
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i]->id == id) return entries[i].get();
    }
    return nullptr;
}

const ParticipantEntry* ParticipantDirectory::byPointer(const Participant* participant) const {
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i]->participant.get() == participant) return entries[i].get();
    }
    return nullptr;
}
