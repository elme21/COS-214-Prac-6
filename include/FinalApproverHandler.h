#ifndef FINAL_APPROVER_HANDLER_H
#define FINAL_APPROVER_HANDLER_H

#include "ApprovalHandler.h"
#include <string>

/// @brief The FinalApproverHandler class is a ConcreteHandler that approves requests up to a certain limit. If the request exceeds the limit, it is denied.
class FinalApproverHandler : public ApprovalHandler
{
public:
    /// @brief Constructs a FinalApproverHandler with a specified limit.
    /// @param limit The maximum amount this handler can approve.
    explicit FinalApproverHandler(std::string& name, double limit) : ApprovalHandler(name, nullptr), limit(limit) {}

    /// @brief Tries to decide on the approval request.
    /// @param request The approval request to decide on.
    /// @return true if the request was handled, false otherwise.
    bool tryDecide(ApprovalRequest& request) override;
    /// @brief Gets the highest value this handler may approve
    double getLimit() const { return limit; }
private:
    /// @brief The maximum amount this handler can approve.
    double limit;
};

#endif