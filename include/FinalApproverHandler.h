#ifndef FINAL_APPROVER_HANDLER_H
#define FINAL_APPROVER_HANDLER_H

#include "ApprovalHandler.h"

class FinalApproverHandler : public ApprovalHandler
{
public:
    explicit FinalApproverHandler(double limit) : ApprovalHandler(nullptr), limit(limit) {}

    void handleRequest(ApprovalRequest& request) override;

private:
    double limit;
};

#endif