#ifndef PARTICIPANT_H
#define PARTICIPANT_H

#include <string>
#include "Colleague.h"

class Participant : public Colleague
{
    public:
        Participant(const std::string& name, WorkCoordinator* coordinator = nullptr);
        virtual ~Participant();

        const std::string& getName() const;
        int workload() const;
        void itemAssigned();
        void itemReleased();

    private:
        std::string name;
        int assignedCount;
};

#endif