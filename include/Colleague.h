#ifndef COLLEAGUE_H
#define COLLEAGUE_H

#include <string>

class WorkCoordinator;

/// @brief The Colleague class is the Colleague of the mediator pattern
class Colleague
{
    public:
        /// @brief The constructor
        /// @param coordinator The work coordinator managing this colleague
        Colleague(WorkCoordinator* coordinator = nullptr);
        /// @brief The destructor
        virtual ~Colleague();

        /// @brief sets the coordinator that is managing the colleaue
        /// @param coordinator the work coordinator being set
        void setCoordinator(WorkCoordinator* coordinator);

        /// @brief notifies the coordinator that this colleague has changed
        /// @param event the event that occurred
        void changed(const std::string& event);

        /// @brief gets overriden by a concrete colleague if needed
        /// @param event the event being recieved
        /// @param source which colleague is sending the event
        virtual void receive(const std::string& event, Colleague* source);

    protected:
        /// @brief The work coordinator managing this colleague
        WorkCoordinator* coordinator;
};

#endif