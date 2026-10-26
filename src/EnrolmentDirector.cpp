
#include "../include/EnrolmentDirector.h"
#include "../include/DefinitionBuilder.h"

void EnrolmentDirector::construct(DefinitionBuilder& builder)
{
    //Root stage
    builder.addStage("UniversityApplication", "");

    //Stage 1: application
    builder.addStage("Application", "UniversityApplication");
    builder.addTask("SubmitApplication", "manual", "Application");
    builder.addTask("CheckDocuments", "manual", "Application");

    //Stage 2: verification
    builder.addStage("Verification", "UniversityApplication");
    builder.addTask("VerifyPersonalDetails", "automated", "Verification");
    builder.addTask("VerifyResults", "automated", "Verification");

    //Stage 3: faculty decision
    builder.addStage("Assessment", "UniversityApplication");
    builder.addTask("FacultyChecksRequirements", "manual", "Assessment");
    builder.addTask("ApplicationDecision", "approval", "Assessment");

    //Stage 4: final notification
    builder.addStage("Notification", "UniversityApplication");
    builder.addTask("NotifyApplicant", "automated", "Notification");

    // dependencies
    builder.addDependency("SubmitApplication", "CheckDocuments");

    //both verification tasks depend on document checking
    builder.addDependency("CheckDocuments", "VerifyPersonalDetails");
    builder.addDependency("CheckDocuments", "VerifyResults");

    //faculty assessment requires BOTH verifications
    builder.addDependency("VerifyPersonalDetails","FacultyChecksRequirements");
    builder.addDependency("VerifyResults","FacultyChecksRequirements");

    //faculty assessment comes before the decision
    builder.addDependency("FacultyChecksRequirements","ApplicationDecision");

    //notification follows the decision
    builder.addDependency("ApplicationDecision", "NotifyApplicant");
}

//Please note this still needs a lot of work. 