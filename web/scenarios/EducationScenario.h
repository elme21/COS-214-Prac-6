#ifndef WEB_EDUCATIONSCENARIO_H
#define WEB_EDUCATIONSCENARIO_H

#include "Scenario.h"
#include "ChainTrace.h"

#include <memory>

/**
 * Industry 1: university application and admission at a mid-sized South African
 * university. The definition comes from the team's EnrolmentDirector, unchanged.
 */
class EducationScenario : public Scenario {
public:
    EducationScenario();

    std::string key() const override { return "education"; }
    std::string title() const override { return "Education - university application"; }
    std::string description() const override;
    std::string subjectNoun() const override { return "applicant"; }
    std::string idPrefix() const override { return "EDU"; }
    std::string directorName() const override { return "EnrolmentDirector"; }
    std::string definitionName() const override { return "University application"; }
    std::vector<std::string> decoratedTasks() const override;

    void construct(DefinitionBuilder& builder) override;
    void seedStaff(Staff& staff) override;
    std::vector<SubjectSeed> subjects() const override;
    std::vector<TaskInfo> tasks() const override;
    std::vector<ChainLink> chain() const override;
    std::string chainTaskId() const override { return "ApplicationDecision"; }

    void configureInstance(RuleContext& context) override;
    Resource* resourceFor(const std::string& taskId, const std::string& participantId, Staff& staff) override;
    bool canStart(RuleContext& context, const std::string& taskId, std::string& reason) override;
    void afterTransition(RuleContext& context, const std::string& taskId,
                         const std::string& newState, const std::string& outcome) override;

private:
    // Approval chain for ApplicationDecision: faculty reviewer -> HOD -> registrar.
    std::unique_ptr<TracingFinalApproverHandler> registrar;
    std::unique_ptr<TracingRoleApproverHandler> hod;
    std::unique_ptr<TracingRoleApproverHandler> facultyReviewer;
};

#endif
