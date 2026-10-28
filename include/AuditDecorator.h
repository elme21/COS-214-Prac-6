/**
 * @file AuditDecorator.h
 * @brief Decorator that records every lifecycle attempt on a task.
 */
#ifndef AUDITDECORATOR_H
#define AUDITDECORATOR_H

#include <functional>
#include <string>
#include "WorkComponentDecorator.h"

/**
 * @brief Records each lifecycle attempt, including refused ones.
 *
 * Place it outermost in a decorator stack so attempts refused by inner
 * decorators (for example a wrong-role approval) are audited too. The sink is
 * wired by the workflow builder, e.g. to the event publisher.
 *
 * @par Design pattern
 * Decorator (ConcreteDecorator role).
 */
class AuditDecorator : public WorkComponentDecorator {
public:
    /** @brief Receives one human-readable line per audited attempt. */
    typedef std::function<void(const std::string&)> Sink;

    /**
     * @brief Wraps @p component with auditing.
     * @param component Component to wrap. The decorator takes ownership.
     * @param taskName Name shown in each audit line.
     * @param sink Receives the audit lines; may be empty to disable output.
     */
    AuditDecorator(WorkComponent* component, const std::string& taskName, Sink sink);

    /** @name Audited operations
     *  Each forwards to the wrapped component, records the outcome, and returns it.
     *  @{ */
    bool assignTo(Participant* participant) override;  ///< Audited assign.
    bool start() override;                             ///< Audited start.
    bool approve() override;                           ///< Audited approve.
    bool reject() override;                            ///< Audited reject.
    bool complete() override;                          ///< Audited complete.
    bool cancel() override;                            ///< Audited cancel.
    bool escalate() override;                          ///< Audited escalate.
    /** @} */

    /** @return A new decorator, owned by the caller, wrapping a deep copy. */
    WorkComponent* clone() const override;

private:
    /**
     * @brief Writes one audit line to the sink.
     * @param action Name of the attempted operation.
     * @param succeeded Whether the transition happened.
     * @return @p succeeded, so callers can return the result directly.
     */
    bool record(const std::string& action, bool succeeded) const;

    std::string taskName;  ///< Task name used in audit lines.
    Sink sink;             ///< Destination of audit lines.
};

#endif
