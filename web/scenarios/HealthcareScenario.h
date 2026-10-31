#ifndef WEB_HEALTHCARESCENARIO_H
#define WEB_HEALTHCARESCENARIO_H

#include "Scenario.h"
#include "RoleApproverHandler.h"
#include "FinalApproverHandler.h"

#include <memory>

/**
 * Industry 2: patient appointment booking at a South African community clinic.
 * About urgency routing and competing for limited doctor slots, not document review.
 * Same engine classes as Education; only definition, people, rules and data differ.
 */
class HealthcareScenario : public Scenario {
public:
    HealthcareScenario();

    std::string key() const override { return "healthcare"; }
    std::string title() const override { return "Healthcare - clinic appointment booking"; }
    std::string description() const override;
    std::string subjectNoun() const override { return "patient"; }
    std::string idPrefix() const override { return "HC"; }
    std::string directorName() const override { return "HealthcareDirector"; }
    std::string definitionName() const override { return "Clinic appointment"; }
    std::vector<std::string> decoratedTasks() const override;

    void construct(DefinitionBuilder& builder) override;
    void seedStaff(Staff& staff) override;
    std::vector<SubjectSeed> subjects() const override;
    std::vector<TaskInfo> tasks() const override;
    std::vector<ChainLink> chain() const override;
    std::string chainTaskId() const override { return "SpecialistReferral"; }

    void configureInstance(RuleContext& context) override;
    Resource* resourceFor(const std::string& taskId, const std::string& participantId, Staff& staff) override;
    bool canStart(RuleContext& context, const std::string& taskId, std::string& reason) override;
    void afterTransition(RuleContext& context, const std::string& taskId,
                         const std::string& newState, const std::string& outcome) override;

private:
    // Referral chain: Nurse -> GP -> Specialist (RoleApproverHandler) -> clinical manager (Final).
    std::unique_ptr<FinalApproverHandler> clinicalManager;
    std::unique_ptr<RoleApproverHandler> specialist;
    std::unique_ptr<RoleApproverHandler> gp;
    std::unique_ptr<RoleApproverHandler> nurse;
};

#endif
