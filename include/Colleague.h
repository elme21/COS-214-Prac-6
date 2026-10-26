#ifndef COLLEAGUE_H
#define COLLEAGUE_H

#include <string>

class WorkCoordinator;

class Colleague
{
    public:
        Colleague(WorkCoordinator* coordinator = nullptr);
        virtual ~Colleague();

        void setCoordinator(WorkCoordinator* coordinator);

        void changed(const std::string& event);

        virtual void receive(const std::string& event, Colleague* source);

    protected:
        WorkCoordinator* coordinator;
};

#endif