#ifndef RESOURCE_H
#define RESOURCE_H

#include "Colleague.h"

class Resource : public Colleague
{
    public:
        Resource(int capacity, WorkCoordinator* coordinator = nullptr);
        virtual ~Resource();

        bool isFree() const;

    private:
        int capacity;
};

#endif