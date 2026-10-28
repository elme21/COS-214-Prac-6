#ifndef RESOURCE_H
#define RESOURCE_H

#include <string>

#include "Colleague.h"
/// @brief 
/// The Resource class is a Concrete Collegeue for the mediator
class Resource : public Colleague
{
    public:
        /// @brief The constructor for the Resource class.
        /// @param name name of the resource
        /// @param capacity maximum number of work items that can use this resource at the same time
        /// @param coordinator pointer to the work coordinator managing this resource
        Resource(const std::string& name, int capacity, WorkCoordinator* coordinator = nullptr);
        /// @brief The destructor for the Resource class.
        virtual ~Resource();

        /// @brief Gets the name of the resource.
        /// @return The name of the resource.
        const std::string& getName() const;
        /// @brief Gets the capacity of the resource.
        /// @return The capacity of the resource.
        int getCapacity() const; // total places, never changes
        /// @brief Gets the number of available places in the resource.
        /// @return The number of available places in the resource.
        int getAvailable() const; // places not currently taken
        /// @brief Checks if the resource is free (i.e., has at least one available place).
        /// @return true if at least one place is left, false otherwise.
        bool isFree() const; // true if at least one place is left

        /// @brief reduces the available space a Resource has
        void take(); //called by the coordinator only
        /// @brief increases the available space a Resource has
        void giveBack(); //called by the coordinator only

    private:
        /// @brief The name of the resource.
        std::string name;
        /// @brief The maximum number of work items that can use this resource at the same time.
        int capacity;
        /// @brief The number of available places in the resource.
        int available;
};

#endif