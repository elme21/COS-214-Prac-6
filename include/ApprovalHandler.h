#ifndef APPROVAL_HANDLER_H
#define APPROVAL_HANDLER_H

class ApprovalRequest;

/// @brief The ApprovalHandler class is the Handler of the chain of responsibility pattern
class ApprovalHandler
{
public:
    /// @brief Constructs an approval handler with a name and an optional successor.
    /// @param name The name of the handler.
    /// @param next The next handler in the chain.
    explicit ApprovalHandler(const std::string& name, ApprovalHandler* next = nullptr)
        : name(name), successor(next) {}
    /// @brief Destroys the approval handler.
    virtual ~ApprovalHandler() {}

    /// @brief Handles an approval request.
    /// @param request The approval request to handle.
    void handleRequest(ApprovalRequest& request);

    /// @brief Sets the successor in the chain of responsibility.
    /// @param next The next handler in the chain.
    void setSuccessor(ApprovalHandler* next) { successor = next; }
    /// @brief Gets the name of the handler.
    /// @return The name of the handler.
    const std::string& getName() const { return name; }

protected:
    /// @brief Tries to decide on an approval request.
    /// @param request The approval request to decide on.
    /// @return True if this handler decided the request, false otherwise.
    virtual bool tryDecide(ApprovalRequest& request) = 0;

private:
    /// @brief The name of the handler.
    std::string name;
    /// @brief The next handler in the chain of responsibility.
    ApprovalHandler* successor;
};
#endif