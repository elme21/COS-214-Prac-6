#ifndef APPROVAL_HANDLER_H
#define APPROVAL_HANDLER_H

class ApprovalHandler
{
    public:
        ApprovalHandler(ApprovalHandler* s) : successor(s) {}
        virtual void handleRequest();
    private:
        ApprovalHandler* successor;
};

#endif