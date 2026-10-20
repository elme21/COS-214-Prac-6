#ifndef FINAL_APPROVER_HANDLER_H
#define FINAL_APPROVER_HANDLER_H

#include "ApprovalHandler.h"

class FinalApproverHandler : public ApprovalHandler
{
    public:
        void handleRequest();
        bool isAuthorized();
};

#endif