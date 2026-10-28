// TEMPORARY - stands in for path reporting by the approval handlers (Member 4) until
// RoleApproverHandler / FinalApproverHandler report which handler a request reached.
// Delete when that lands.
#include "ChainTrace.h"

#include "ApprovalRequest.h"

ChainTrace::Sink& ChainTrace::sink() {
    static Sink theSink;
    return theSink;
}

void ChainTrace::setSink(const Sink& newSink) { sink() = newSink; }

void ChainTrace::clearSink() { sink() = Sink(); }

void ChainTrace::report(const std::string& handler, double limit, double value, bool isFinal) {
    if (sink()) sink()(handler, limit, value, isFinal);
}

void TracingRoleApproverHandler::handleRequest(ApprovalRequest& request) {
    ChainTrace::report(role, limit, request.getValue(), false);
    RoleApproverHandler::handleRequest(request);
}

void TracingFinalApproverHandler::handleRequest(ApprovalRequest& request) {
    ChainTrace::report(role, limit, request.getValue(), true);
    FinalApproverHandler::handleRequest(request);
}
