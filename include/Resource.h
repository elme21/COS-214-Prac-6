#ifndef RESOURCE_H
#define RESOURCE_H

#include <string>

#include "Colleague.h"

class Resource : public Colleague
{
    public:
        Resource(const std::string& name, int capacity, WorkCoordinator* coordinator = nullptr);
        virtual ~Resource();

        const std::string& getName() const;
        int getCapacity() const; // total places, never changes
        int getAvailable() const; // places not currently taken
        bool isFree() const; // true if at least one place is left

        void take(); //called by the coordinator only
        void giveBack(); //called by the coordinator only

    private:
        std::string name;
        int capacity;
        int available;
};

#endif