// TEMPORARY - stands in for a name and role on Participant (Member 4) until Participant
// carries its own identity (getName()/getRole()). Delete when that lands; keep only the
// seeding of Participant and Resource objects, which is industry data.
#ifndef WEB_PARTICIPANTDIRECTORY_H
#define WEB_PARTICIPANTDIRECTORY_H

#include <memory>
#include <string>
#include <vector>

class Participant;
class Resource;
class ResourceCoordinator;

/** One seeded person: the real engine Participant plus display data the engine lacks. */
struct ParticipantEntry {
    std::string id;
    std::string name;
    std::string role;
    std::unique_ptr<Participant> participant;   // real engine colleague
    std::unique_ptr<Resource> slots;            // real engine Resource, or null
    int slotCapacity;                           // value passed to Resource(capacity), for display
};

class ParticipantDirectory {
public:
    explicit ParticipantDirectory(ResourceCoordinator* coordinator);
    ~ParticipantDirectory();

    /** Creates a Participant (and optionally a Resource) and registers both with the Mediator. */
    ParticipantEntry& add(const std::string& id, const std::string& name, const std::string& role,
                          int slotCapacity = -1);

    ParticipantEntry* find(const std::string& id);
    const ParticipantEntry* byPointer(const Participant* participant) const;
    const std::vector<std::unique_ptr<ParticipantEntry> >& all() const { return entries; }

private:
    ResourceCoordinator* coordinator;
    std::vector<std::unique_ptr<ParticipantEntry> > entries;
};

#endif
