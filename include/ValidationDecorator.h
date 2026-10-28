/**
 * @file ValidationDecorator.h
 * @brief Decorator that refuses approval or completion until a rule passes.
 */
#ifndef VALIDATIONDECORATOR_H
#define VALIDATIONDECORATOR_H

#include <functional>
#include "WorkComponentDecorator.h"

/**
 * @brief Blocks approve() and complete() until a validation rule is satisfied.
 *
 * Example: an enrolment task cannot be completed until all required documents
 * are present. The rule is supplied by whoever assembles the workflow, so the
 * task classes stay free of validation logic.
 *
 * @par Design pattern
 * Decorator (ConcreteDecorator role).
 */
class ValidationDecorator : public WorkComponentDecorator {
public:
    /** @brief Predicate over the wrapped component; true means it may proceed. */
    typedef std::function<bool(const WorkComponent&)> Rule;

    /**
     * @brief Wraps @p component with a validation rule.
     * @param component Component to wrap. The decorator takes ownership.
     * @param rule Checked before approve() and complete().
     */
    ValidationDecorator(WorkComponent* component, Rule rule);

    /**
     * @brief Approves only if the rule passes.
     * @return false if the rule fails or the wrapped component refuses.
     */
    bool approve() override;

    /**
     * @brief Completes only if the rule passes.
     * @return false if the rule fails or the wrapped component refuses.
     */
    bool complete() override;

    /** @return A new decorator, owned by the caller, wrapping a deep copy. */
    WorkComponent* clone() const override;

private:
    Rule rule;  ///< Validation rule applied before approve() and complete().
};

#endif
