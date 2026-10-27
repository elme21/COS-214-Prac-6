#ifndef APPROVAL_HANDLER_H
#define APPROVAL_HANDLER_H

class ApprovalRequest;

/// @brief The ApprovalHandler class is the Handler of the chain of responsibility pattern
class ApprovalHandler
{
public:
    /// @brief The constructor for the ApprovalHandler class. It initializes the successor in the chain of responsibility.
    /// @param next The next handler in the chain
    explicit ApprovalHandler(ApprovalHandler* next = nullptr) : successor(next) {}
    /// @brief The destructor for the ApprovalHandler class. It is declared virtual to ensure proper cleanup of derived classes.
    virtual ~ApprovalHandler() {}

    // Default behaviour: pass the request to the next link.
    /// @brief It passes the request to the next handler in the chain.
    /// @param request The approval request to handle
    virtual void handleRequest(ApprovalRequest& request);

    /// @brief Sets the successor in the chain of responsibility.
    /// @param next The next handler in the chain
    void setSuccessor(ApprovalHandler* next) { successor = next; }

private:
    /// @brief A pointer to the next handler in the chain of responsibility.
    ApprovalHandler* successor;   // non-owning
};

#endif