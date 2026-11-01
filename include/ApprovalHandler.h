#ifndef APPROVAL_HANDLER_H
#define APPROVAL_HANDLER_H

class ApprovalRequest;
#include <string>

/// @brief The ApprovalHandler class is the Handler of the chain of responsibility pattern
class ApprovalHandler
{
public:
    /// @brief The constructor for the ApprovalHandler class. It initializes the successor in the chain of responsibility.
    /// @param name The name of the handler
    /// @param next The next handler in the chain
    explicit ApprovalHandler(const std::string& name, ApprovalHandler* next = nullptr) : name(name), successor(next) {}
    /// @brief The destructor for the ApprovalHandler class. It is declared virtual to ensure proper cleanup of derived classes.
    virtual ~ApprovalHandler() {}

    // Default behaviour: pass the request to the next link.
    /// @brief It passes the request to the next handler in the chain.
    /// @param request The approval request to handle
    void handleRequest(ApprovalRequest& request);

    /// @brief Sets the successor in the chain of responsibility.
    /// @param next The next handler in the chain
    void setSuccessor(ApprovalHandler* next) { successor = next; }
    /// @brief Gets the name of the handler
    /// @return The handler's name
    const std::string& getName() const { return name; }

protected:
    /// @brief Attempts to decide the approval request
    /// @param request The approval request to decide
    /// @return True if the request was decided, false otherwise
    virtual bool tryDecide(ApprovalRequest& request) = 0;

private:
    /// @brief The name of the handler
    std::string name;
    /// @brief A pointer to the next handler in the chain of responsibility.
    ApprovalHandler* successor;   // non-owning
};

#endif