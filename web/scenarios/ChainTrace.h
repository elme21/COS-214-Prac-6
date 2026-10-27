// TEMPORARY - stands in for approval events published through EventPublisher (Member 6)
// until the chain's handlers publish "request reached handler X" events that an
// EventListener can show. Delete when that lands (use plain RoleApproverHandler /
// FinalApproverHandler in the directors again).
#ifndef WEB_CHAINTRACE_H
#define WEB_CHAINTRACE_H

#include "RoleApproverHandler.h"
#include "FinalApproverHandler.h"

#include <functional>
#include <string>

/**
 * Where tracing handlers report to. The gateway points this at the current
 * action's trace while ApprovalTask::execute() runs, and clears it afterwards.
 */
class ChainTrace {
public:
    typedef std::function<void(const std::string& handler, double limit, double value, bool isFinal)> Sink;
    static void setSink(const Sink& sink);
    static void clearSink();
    static void report(const std::string& handler, double limit, double value, bool isFinal);

private:
    static Sink& sink();
};

/**
 * A RoleApproverHandler that reports when a request reaches it, then lets the
 * real RoleApproverHandler::handleRequest decide. No chain logic lives here.
 */
class TracingRoleApproverHandler : public RoleApproverHandler {
public:
    TracingRoleApproverHandler(const std::string& role, double limit, ApprovalHandler* next = nullptr)
        : RoleApproverHandler(role, limit, next), role(role), limit(limit) {}

    void handleRequest(ApprovalRequest& request) override;

    const std::string& getRole() const { return role; }
    double getLimit() const { return limit; }

private:
    // Copies for display only: RoleApproverHandler keeps its own fields private.
    std::string role;
    double limit;
};

/** A FinalApproverHandler that reports when a request reaches it, then lets it decide. */
class TracingFinalApproverHandler : public FinalApproverHandler {
public:
    TracingFinalApproverHandler(const std::string& role, double limit)
        : FinalApproverHandler(limit), role(role), limit(limit) {}

    void handleRequest(ApprovalRequest& request) override;

    const std::string& getRole() const { return role; }
    double getLimit() const { return limit; }

private:
    std::string role;
    double limit;
};

#endif
