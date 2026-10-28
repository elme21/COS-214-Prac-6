#ifndef WEB_STAFF_H
#define WEB_STAFF_H

#include <memory>
#include <string>
#include <vector>

class Participant;
class Resource;

/** One seeded person: a real engine Participant, plus an optional real Resource (their bookable slots). */
struct StaffMember {
    std::unique_ptr<Participant> person;
    std::unique_ptr<Resource> slots;   // null if this person has no limited slots
};

/**
 * Industry data: owns the seeded Participant and Resource objects of one industry.
 * Names, roles, capacity and free places all live on the engine objects themselves.
 */
class Staff {
public:
    Staff();
    ~Staff();

    /** Creates a real Participant(id, name, role) and, if slotCapacity >= 0, a real Resource for their slots. */
    Participant& add(const std::string& id, const std::string& name, const std::string& role, int slotCapacity = -1);

    Participant* find(const std::string& id) const;
    Resource* slotsOf(const std::string& id) const;
    const std::vector<std::unique_ptr<StaffMember> >& members() const { return people; }

private:
    std::vector<std::unique_ptr<StaffMember> > people;
};

#endif
