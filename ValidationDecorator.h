#ifndef VALIDATIONDECORATOR_H
#define VALIDATIONDECORATOR_H

#include <functional>
#include "WorkComponentDecorator.h"

// Blocks approve/complete until the supplied rule is satisfied
// (for example "all required enrolment documents are present").
class ValidationDecorator : public WorkComponentDecorator {
public:
    typedef std::function<bool(const WorkComponent&)> Rule;

    ValidationDecorator(WorkComponent* component, Rule rule);

    bool approve() override;
    bool complete() override;
    WorkComponent* clone() const override;

private:
    Rule rule;
};

#endif