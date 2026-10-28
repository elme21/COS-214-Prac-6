/**
 * @file SecureDecorator.cpp
 * @brief Implementation of SecureDecorator.
 */
#include "SecureDecorator.h"
#include "Participant.h"

SecureDecorator::SecureDecorator(WorkComponent* component, const std::string& role)
    : WorkComponentDecorator(component), requiredRole(role) {}

bool SecureDecorator::isPermitted(const Participant* participant) const {
    return participant != nullptr && participant->getRole() == requiredRole;
}

bool SecureDecorator::assignTo(Participant* participant) {
    return isPermitted(participant) && WorkComponentDecorator::assignTo(participant);
}

bool SecureDecorator::approve() {
    return isPermitted(getAssignee()) && WorkComponentDecorator::approve();
}

bool SecureDecorator::reject() {
    return isPermitted(getAssignee()) && WorkComponentDecorator::reject();
}

bool SecureDecorator::complete() {
    return isPermitted(getAssignee()) && WorkComponentDecorator::complete();
}

WorkComponent* SecureDecorator::clone() const {
    return new SecureDecorator(*this);
}
