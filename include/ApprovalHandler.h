#ifndef APPROVAL_HANDLER_H
#define APPROVAL_HANDLER_H

class ApprovalRequest;

class ApprovalHandler
{
public:
    explicit ApprovalHandler(ApprovalHandler* next = nullptr) : successor(next) {}
    virtual ~ApprovalHandler() {}

    // Default behaviour: pass the request to the next link.
    virtual void handleRequest(ApprovalRequest& request);

    void setSuccessor(ApprovalHandler* next) { successor = next; }

private:
    ApprovalHandler* successor;   // non-owning
};

#endif