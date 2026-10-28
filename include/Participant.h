#ifndef PARTICIPANT_H
#define PARTICIPANT_H

#include <string>
#include "Colleague.h"

/// @brief
/// The Participant class is a Concrete Collegeue for the mediator
class Participant : public Colleague
{
    public:
        /// @brief The constructor
        /// @param id the participants id
        /// @param name the participants name
        /// @param role the participants role
        /// @param coordinator pointer to the WorkCoordinator managing this participant
        Participant(const std::string& id, const std::string& name, const std::string& role, WorkCoordinator* coordinator = nullptr);
        /// @brief The destructor
        virtual ~Participant();

        /// @brief gets the participants ID
        /// @return the participants ID
        const std::string& getID() const;
        /// @brief gets the participants name
        /// @return the participants name
        const std::string& getName() const;
        /// @brief gets the participants role
        /// @return the participants role
        const std::string& getRole() const;
        
        /// @brief gets the number of work items currently assigned to this participant
        /// @return the number of work items currently assigned to this participant
        int workload() const;
        /// @brief increments the number of work items currently assigned to this participant
        void itemAssigned();
        /// @brief decrements the number of work items currently assigned to this participant
        void itemReleased();

    private:
        /// @brief The participants ID
        std::string id;
        /// @brief The participants name
        std::string name;
        /// @brief The participants role
        std::string role;
        /// @brief The number of work items assigned to this participant
        int assignedCount;
};

#endif