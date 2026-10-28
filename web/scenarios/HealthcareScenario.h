#ifndef WEB_HEALTHCARESCENARIO_H
#define WEB_HEALTHCARESCENARIO_H

#include "Scenario.h"
#include "ChainTrace.h"

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

    std::shared_ptr<WorkflowDefinition> buildDefinition(RecordingDefinitionBuilder& builder) override;
    void seedParticipants(ParticipantDirectory& directory) override;
    std::vector<SubjectSeed> subjects() const override;
    std::vector<TaskInfo> tasks() const override;
    std::vector<ChainLink> chain() const override;
    std::string chainTaskId() const override { return "SpecialistReferral"; }

    void configureInstance(RuleContext& context) override;
    CaseData connectorContext(RuleContext& context, const std::string& taskId) override;
    Resource* resourceFor(const std::string& taskId, ParticipantEntry& who) override;
    bool canStart(RuleContext& context, const std::string& taskId, std::string& reason) override;
    void afterTransition(RuleContext& context, const std::string& taskId, const std::string& action,
                         const std::string& newState, const std::string& outcome) override;

private:
    // Referral chain: Nurse -> GP -> Specialist (RoleApproverHandler) -> clinical manager (Final).
    std::unique_ptr<TracingFinalApproverHandler> clinicalManager;
    std::unique_ptr<TracingRoleApproverHandler> specialist;
    std::unique_ptr<TracingRoleApproverHandler> gp;
    std::unique_ptr<TracingRoleApproverHandler> nurse;
};

#endif
