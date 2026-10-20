#ifndef PARTICIPANT_H
#define PARTICIPANT_H

#include "Colleague.h"

class Participant : public Colleague
{
    public:
        Participant(WorkCoordinator* coordinator = nullptr);
        virtual ~Participant();

        int workload() const;
        void itemAssigned();
        void itemReleased();
    private:
        int assignedCount;
};

#endif