#include "EducationScenario.h"

#include "RuleHelpers.h"
#include "Staff.h"
#include "Trace.h"

#include "DefinitionBuilder.h"
#include "EnrolmentDirector.h"
#include "AutomatedTask.h"
#include "ApprovalTask.h"
#include "WorkItem.h"

#include <cstdlib>

EducationScenario::EducationScenario()
    : registrar(new TracingFinalApproverHandler("Registrar", 85)),
      hod(new TracingRoleApproverHandler("Head of Department", 60, registrar.get())),
      facultyReviewer(new TracingRoleApproverHandler("Faculty reviewer", 30, hod.get())) {}

std::string EducationScenario::description() const {
    return "Applicants submit documents; admissions checks them (looping back while documents are "
           "missing); identity and results are verified in parallel; the faculty checks programme "
           "requirements; an approval chain admits or rejects; the applicant is notified.";
}

std::vector<std::string> EducationScenario::decoratedTasks() const {
    return std::vector<std::string>(1, "ApplicationDecision");
}

void EducationScenario::construct(DefinitionBuilder& builder) {
    EnrolmentDirector director;          // the team's Director, unchanged
    director.construct(builder);
    // EnrolmentDirector has no approval chain or decorator steps, so the industry adds them here.
    builder.setApprovalChain("ApplicationDecision", facultyReviewer.get());
    builder.addDecorator("ApplicationDecision", "audit");
}

void EducationScenario::seedStaff(Staff& s) {
    s.add("applicant", "Applicant (self-service portal)", "Applicant");
    s.add("khumalo", "Ms Lindiwe Khumalo", "Admissions officer");
    s.add("steyn", "Mr Pieter Steyn", "Admissions officer");
    s.add("botha", "Dr Johan Botha", "Faculty reviewer");
    s.add("zulu", "Prof Nomsa Zulu", "Head of Department");
    s.add("govender", "Mr Ravi Govender", "Registrar");
    s.add("system", "TIVIDY automation", "System (automated steps)");
}

namespace {
SubjectSeed applicant(const std::string& id, const std::string& name, const std::string& summary,
                      const std::string& programme, const std::string& curriculum, const std::string& aps,
                      const std::string& exam, const std::string& idNumber, const std::string& cell,
                      const std::string& risk, const std::string& documents) {
    SubjectSeed s;
    s.id = id;
    s.name = name;
    s.summary = summary;
    s.data["name"] = name;
    s.data["programme"] = programme;
    s.data["curriculum"] = curriculum;
    s.data["aps"] = aps;
    s.data["examNumber"] = exam;
    s.data["idNumber"] = idNumber;
    s.data["cellphone"] = cell;
    s.data["riskScore"] = risk;
    s.data["documents"] = documents;
    return s;
}

TaskInfo task(const std::string& id, const std::string& label, const std::string& role, const std::string& help) {
    TaskInfo t;
    t.id = id;
    t.label = label;
    t.roleHint = role;
    t.help = help;
    return t;
}
}

std::vector<SubjectSeed> EducationScenario::subjects() const {
    std::vector<SubjectSeed> list;
    list.push_back(applicant("thandiwe", "Thandiwe Mokoena",
        "BSc Computer Science, NSC, APS 42 - strong applicant (risk 15)",
        "BSc Computer Science", "NSC", "42", "NSC-2025-118204", "0703140801084", "0821234567", "15", "complete"));
    list.push_back(applicant("pieter", "Pieter van Wyk",
        "BCom Accounting, NSC, APS 33 - borderline, needs the HOD (risk 48)",
        "BCom Accounting", "NSC", "33", "NSC-2025-207733", "0609225115086", "0739876543", "48", "complete"));
    list.push_back(applicant("aisha", "Aisha Patel",
        "BEng Mechanical, Cambridge A-levels - automated results check fails; registrar decides (risk 72)",
        "BEng Mechanical Engineering", "Cambridge International", "40", "CIE-ZA-55102", "0705070140185",
        "0605551234", "72", "complete"));
    list.push_back(applicant("sipho", "Sipho Dlamini",
        "BSc Actuarial Science, NSC, APS 26 (minimum 40) - chain rejects (risk 91)",
        "BSc Actuarial Science", "NSC", "26", "NSC-2025-330911", "0611305788089", "0817654321", "91", "complete"));
    list.push_back(applicant("kagiso", "Kagiso Sithole",
        "LLB, NSC, APS 38 - certified ID copy missing, so Check Documents loops (risk 22)",
        "LLB", "NSC", "38", "NSC-2025-412870", "0708190307080", "0724567890", "22", "certified ID copy missing"));
    return list;
}

std::vector<TaskInfo> EducationScenario::tasks() const {
    std::vector<TaskInfo> list;
    list.push_back(task("SubmitApplication", "Submit Application", "Applicant",
                        "Applicant uploads documents and personal details."));
    list.push_back(task("CheckDocuments", "Check Documents", "Admissions officer",
                        "Documents missing? reject() it (loop back), then reassign once they arrive."));
    list.push_back(task("VerifyPersonalDetails", "Verify Personal Details", "System (automated steps)",
                        "Parallel branch: automated identity check."));
    list.push_back(task("VerifyResults", "Verify Results", "System (automated steps)",
                        "Parallel branch: automated NSC results check. Fails for non-NSC curricula: escalate, "
                        "then an admissions officer finishes it by hand."));
    list.push_back(task("FacultyChecksRequirements", "Faculty Checks Requirements", "Faculty reviewer",
                        "Join: becomes available only when BOTH verifications are complete."));
    list.push_back(task("ApplicationDecision", "Approve or Reject Application", "Faculty reviewer",
                        "Wrapped in an AuditDecorator. execute() sends the risk score down Faculty reviewer -> "
                        "HOD -> Registrar."));
    list.push_back(task("NotifyApplicant", "Notify Applicant", "System (automated steps)",
                        "Automated notification, for either decision."));
    return list;
}

std::vector<ChainLink> EducationScenario::chain() const {
    std::vector<ChainLink> links;
    ChainLink a = { facultyReviewer->getRole(), facultyReviewer->getLimit(), false };
    ChainLink b = { hod->getRole(), hod->getLimit(), false };
    ChainLink c = { registrar->getRole(), registrar->getLimit(), true };
    links.push_back(a);
    links.push_back(b);
    links.push_back(c);
    return links;
}

void EducationScenario::configureInstance(RuleContext& context) {
    CaseData& data = context.data();
    ApprovalTask* decision = asApprovalTask(context.task("ApplicationDecision"));
    if (decision != nullptr) {
        decision->setRequestValue(std::atof(valueOr(data, "riskScore", "0").c_str()));
        context.trace().engine("ApprovalTask::setRequestValue(" + valueOr(data, "riskScore", "0") + ")",
                               "Chain of Responsibility", "admission risk score travels down the chain");
    }
    // Only National Senior Certificate results are in the results database.
    AutomatedTask* results = asAutomatedTask(context.task("VerifyResults"));
    if (results != nullptr && valueOr(data, "curriculum", "") != "NSC") {
        results->setStepSucceeds(false);
        context.trace().scenario("EducationScenario rule", "industry data",
                                 data["curriculum"] + " results are not in the NSC database: "
                                 "AutomatedTask::setStepSucceeds(false) on Verify Results");
    }
}

Resource* EducationScenario::resourceFor(const std::string&, const std::string&, Staff&) { return nullptr; }

bool EducationScenario::canStart(RuleContext&, const std::string&, std::string&) { return true; }

void EducationScenario::afterTransition(RuleContext& context, const std::string& taskId,
                                        const std::string& newState, const std::string&) {
    CaseData& data = context.data();
    if (taskId == "CheckDocuments" && newState == "Rejected") {
        data["documents"] = "missing - waiting for availability";
        context.trace().scenario("EducationScenario rule", "routing (decision)",
            "[Availability = NO] -> loop back: the rejected task waits; assignTo() again once documents arrive "
            "(RejectedState allows reassignment)");
    } else if (taskId == "CheckDocuments" && newState == "Completed") {
        data["documents"] = "complete";
        context.trace().scenario("EducationScenario rule", "routing (decision)",
            "[Availability = YES] -> fork: both verification tasks are dependants of Check Documents");
    } else if (taskId == "ApplicationDecision" && newState == "Completed") {
        data["decision"] = "Admitted";
        data["outcome"] = "Admitted to " + data["programme"];
        context.trace().scenario("EducationScenario rule", "routing (decision)",
            "[Approved = YES] -> Notify Applicant unlocks through its dependency");
    } else if (taskId == "ApplicationDecision" && newState == "Rejected") {
        data["decision"] = "Not admitted";
        data["outcome"] = "Application rejected";
        context.trace().scenario("EducationScenario rule", "routing (decision)",
            "[Approved = NO] -> both outcomes merge into Notify Applicant");
        makeAvailableByRule(context, "NotifyApplicant", "rejection still notifies the applicant");
    } else if (taskId == "NotifyApplicant" && newState == "Completed") {
        data["notified"] = "yes";
    }
}
