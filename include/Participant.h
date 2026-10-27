#ifndef PARTICIPANT_H
#define PARTICIPANT_H

#include <string>
#include "Colleague.h"

class Participant : public Colleague
{
    public:
        Participant(const std::string& id, const std::string& name, const std::string& role, WorkCoordinator* coordinator = nullptr);
        virtual ~Participant();

        const std::string& getID() const;
        const std::string& getName() const;
        const std::string& getRole() const;
        int workload() const;
        void itemAssigned();
        void itemReleased();

    private:
        std::string id;
        std::string name;
        std::string role;
        int assignedCount;
};

#endif