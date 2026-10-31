#ifndef SECUREDECORATOR_H
#define SECUREDECORATOR_H

#include <string>
#include "WorkComponentDecorator.h"

// Only lets participants holding the required role be assigned, and only lets
// the assigned participant act (approve/reject/complete) if they hold it.
class SecureDecorator : public WorkComponentDecorator {
public:
    SecureDecorator(WorkComponent* component, const std::string& requiredRole);

    bool assignTo(Participant* participant) override;
    bool approve() override;
    bool reject() override;
    bool complete() override;
    WorkComponent* clone() const override;

private:
    bool isPermitted(const Participant* participant) const;

    std::string requiredRole;
};

#endif