#ifndef ROLE_APPROVER_HANDLER_H
#define ROLE_APPROVER_HANDLER_H

#include "ApprovalHandler.h"

class RoleApproverHandler : public ApprovalHandler
{
    public:
        void handleRequest();
        bool isAuthorized();
};

#endif