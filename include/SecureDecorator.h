#ifndef SECUREDECORATOR_H
#define SECUREDECORATOR_H

#include <string>
#include "WorkComponentDecorator.h"

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
