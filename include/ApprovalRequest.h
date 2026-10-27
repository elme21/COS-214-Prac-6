#ifndef APPROVALREQUEST_H
#define APPROVALREQUEST_H

class WorkItem;

/// @brief ApprovalRequest class is an inbetween class that allows work item to interact with the chain of responsibility more cleanly
class ApprovalRequest
{
public:
    /// @brief The constructor
    /// @param item The work item requesting to enter the chain
    /// @param value The amount requesting approval
    ApprovalRequest(WorkItem* item, double value = 0.0)
        : item(item), value(value) {}

    /// @brief Gets the work item associated with this approval request
    /// @return A pointer to the work item
    WorkItem* getItem() const { return item; }
    /// @brief Gets the value of this approval request
    /// @return The approval value
    double getValue() const { return value; }

private:
    /// @brief The work item associated with this approval request
    WorkItem* item;
    /// @brief The value of this approval request
    double value;
};

#endif