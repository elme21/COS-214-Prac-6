/**
 * @file SecureDecorator.h
 * @brief Decorator that restricts who may be assigned to and act on a task.
 */
#ifndef SECUREDECORATOR_H
#define SECUREDECORATOR_H

#include <string>
#include "WorkComponentDecorator.h"

/**
 * @brief Role-based access control for a single task.
 *
 * Only participants holding the required role can be assigned, and only the
 * assigned participant, if they hold it, can approve, reject or complete.
 * Requires Participant::hasRole(const std::string&) const.
 *
 * @par Design pattern
 * Decorator (ConcreteDecorator role).
 */
class SecureDecorator : public WorkComponentDecorator {
public:
    /**
     * @brief Wraps @p component with a role requirement.
     * @param component Component to wrap. The decorator takes ownership.
     * @param requiredRole Role a participant must hold, e.g. "registrar".
     */
    SecureDecorator(WorkComponent* component, const std::string& requiredRole);

    /**
     * @brief Assigns only a participant who holds the required role.
     * @param participant Candidate assignee.
     * @return false if @p participant lacks the role or the wrapped component refuses.
     */
    bool assignTo(Participant* participant) override;

    /** @return false unless the current assignee holds the role and the wrapped component accepts. */
    bool approve() override;

    /** @return false unless the current assignee holds the role and the wrapped component accepts. */
    bool reject() override;

    /** @return false unless the current assignee holds the role and the wrapped component accepts. */
    bool complete() override;

    /** @return A new decorator, owned by the caller, wrapping a deep copy. */
    WorkComponent* clone() const override;

private:
    /** @return true if @p participant is non-null and holds the required role. */
    bool isPermitted(const Participant* participant) const;

    std::string requiredRole;  ///< Role needed to be assigned and to act.
};

#endif
