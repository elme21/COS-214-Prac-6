/**
 * @file ValidationDecorator.cpp
 * @brief Implementation of ValidationDecorator.
 */
#include "ValidationDecorator.h"

ValidationDecorator::ValidationDecorator(WorkComponent* component, Rule r)
    : WorkComponentDecorator(component), rule(r) {}

bool ValidationDecorator::approve() {
    return rule(*inner()) && WorkComponentDecorator::approve();
}

bool ValidationDecorator::complete() {
    return rule(*inner()) && WorkComponentDecorator::complete();
}

WorkComponent* ValidationDecorator::clone() const {
    return new ValidationDecorator(*this);
}
