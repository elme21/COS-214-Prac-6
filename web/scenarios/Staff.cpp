#include "Staff.h"

#include "Participant.h"
#include "Resource.h"

Staff::Staff() {}

Staff::~Staff() {}

Participant& Staff::add(const std::string& id, const std::string& name, const std::string& role, int slotCapacity) {
    std::unique_ptr<StaffMember> member(new StaffMember());
    member->person.reset(new Participant(id, name, role));
    if (slotCapacity >= 0) member->slots.reset(new Resource(name + " - appointment slots", slotCapacity));
    people.push_back(std::move(member));
    return *people.back()->person;
}

Participant* Staff::find(const std::string& id) const {
    for (size_t i = 0; i < people.size(); ++i) {
        if (people[i]->person->getID() == id) return people[i]->person.get();
    }
    return nullptr;
}

Resource* Staff::slotsOf(const std::string& id) const {
    for (size_t i = 0; i < people.size(); ++i) {
        if (people[i]->person->getID() == id) return people[i]->slots.get();
    }
    return nullptr;
}
