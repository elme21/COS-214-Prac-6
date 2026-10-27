#include "HealthcareScenario.h"

#include "HealthcareDirector.h"
#include "RecordingDefinitionBuilder.h"
#include "ParticipantDirectory.h"
#include "RuleHelpers.h"
#include "NameLookup.h"
#include "Trace.h"

#include "WorkflowDefinition.h"
#include "WorkItem.h"
#include "ApprovalTask.h"
#include "Resource.h"

#include <cstdlib>

HealthcareScenario::HealthcareScenario()
    : clinicalManager(new TracingFinalApproverHandler("Clinical manager (scheme limit)", 25000)),
      specialist(new TracingRoleApproverHandler("Specialist", 12000, clinicalManager.get())),
      gp(new TracingRoleApproverHandler("GP", 2500, specialist.get())),
      nurse(new TracingRoleApproverHandler("Nurse", 500, gp.get())) {}

std::string HealthcareScenario::description() const {
    return "Reception registers the patient and the medical aid is pre-authorised; a nurse triages. "
           "Urgent patients go straight to the doctor on duty; routine patients compete for a limited "
           "doctor slot, get an SMS reminder, see a GP, and a specialist referral is approved by "
           "Nurse -> GP -> Specialist depending on its cost.";
}

std::shared_ptr<WorkflowDefinition> HealthcareScenario::buildDefinition(RecordingDefinitionBuilder& builder) {
    std::map<std::string, std::string> labels;
    std::vector<TaskInfo> info = tasks();
    for (size_t i = 0; i < info.size(); ++i) labels[info[i].id] = info[i].label;
    builder.setTaskLabels(labels);

    HealthcareDirector director(nurse.get());
    director.construct(builder);
    return builder.getDefinition();
}

void HealthcareScenario::seedParticipants(ParticipantDirectory& d) {
    d.add("mthembu", "Nandi Mthembu", "Receptionist");
    d.add("mahlangu", "Sister Grace Mahlangu", "Nurse");
    d.add("dube", "Sister Zanele Dube", "Nurse");
    // GPs own a Resource each: their bookable slots today (Resource capacity).
    d.add("naidoo", "Dr Kavitha Naidoo", "GP", 0);
    d.add("mokoena", "Dr Themba Mokoena", "GP", 2);
    d.add("vdmerwe", "Dr Anri van der Merwe", "GP", 1);
    d.add("sibiya", "Dr Sizwe Sibiya", "Doctor on duty (emergencies)");
    d.add("adams", "Dr Farouk Adams", "Specialist physician");
    d.add("system", "TIVIDY automation", "System (automated tasks)");
}

namespace {
SubjectSeed patient(const std::string& id, const std::string& name, const std::string& summary,
                    const std::string& age, const std::string& complaint, const std::string& scheme,
                    const std::string& member, const std::string& status, const std::string& cell,
                    const std::string& urgency, const std::string& referral, const std::string& value) {
    SubjectSeed s;
    s.id = id;
    s.name = name;
    s.summary = summary;
    s.data["name"] = name;
    s.data["age"] = age;
    s.data["complaint"] = complaint;
    s.data["medicalAid"] = scheme;
    s.data["memberNumber"] = member;
    s.data["medicalAidStatus"] = status;
    s.data["cellphone"] = cell;
    s.data["suggestedUrgency"] = urgency;
    s.data["referral"] = referral;
    s.data["referralValue"] = value;
    return s;
}
}

std::vector<SubjectSeed> HealthcareScenario::subjects() const {
    std::vector<SubjectSeed> list;
    list.push_back(patient("lerato", "Lerato Molefe",
        "58, chest pain and shortness of breath - triage: urgent",
        "58", "Chest pain, shortness of breath", "Discovery Health", "DH-30419927", "active",
        "0823344556", "urgent", "Cardiology", "9800"));
    list.push_back(patient("johan", "Johan Pretorius",
        "34, knee injury - routine; physio referral R450 (nurse approves)",
        "34", "Knee pain after rugby", "Bonitas", "BN-77120345", "active",
        "0794455667", "routine", "Physiotherapy", "450"));
    list.push_back(patient("mandla", "Mandla Zungu",
        "61, hypertension follow-up - routine; cardiology R2 200 (GP approves)",
        "61", "Blood pressure follow-up", "GEMS", "GE-00982211", "active",
        "0836677889", "routine", "Cardiology", "2200"));
    list.push_back(patient("bongani", "Bongani Nkosi",
        "45, chronic back pain - routine; MRI R9 800 (specialist approves)",
        "45", "Chronic lower back pain", "Momentum Health", "MM-55410098", "active",
        "0717788990", "routine", "MRI lumbar spine", "9800"));
    list.push_back(patient("fatima", "Fatima Ebrahim",
        "29, migraines - medical aid SUSPENDED; R31 500 referral rejected by chain",
        "29", "Recurring migraines", "Medshield", "MS-12003344", "suspended",
        "0648899001", "routine", "Neurology + experimental treatment", "31500"));
    return list;
}

namespace {
TaskInfo task(const std::string& id, const std::string& label, const std::string& role,
              const std::string& connector, const std::string& help) {
    TaskInfo t;
    t.id = id;
    t.label = label;
    t.roleHint = role;
    t.connector = connector;
    t.help = help;
    return t;
}
}

std::vector<TaskInfo> HealthcareScenario::tasks() const {
    std::vector<TaskInfo> list;
    list.push_back(task("RegisterPatient", "Register Patient", "Receptionist", "",
                        "Reception captures the patient's details."));
    list.push_back(task("VerifyMedicalAid", "Verify Medical Aid", "System (automated tasks)", "medical-aid",
                        "Pre-authorisation through the payment (medical-aid) adapter. Declined? escalate, then "
                        "reassign to reception as a cash patient."));
    TaskInfo triage = task("TriageAssessment", "Triage Assessment", "Nurse", "",
                           "ROUTING POINT: the outcome picks the urgent or the booking branch.");
    triage.outcomeQuestion = "Triage outcome";
    triage.outcomes.push_back("routine");
    triage.outcomes.push_back("urgent");
    list.push_back(triage);
    list.push_back(task("EmergencyConsultation", "Emergency Consultation", "Doctor on duty (emergencies)", "",
                        "Urgent branch only."));
    list.push_back(task("BookAppointment", "Book Appointment Slot", "GP", "",
                        "Assigning to a GP requests that GP's slot Resource from the ResourceCoordinator. "
                        "No slot? escalate, then reassign to another GP."));
    list.push_back(task("SendReminder", "Send Reminder", "System (automated tasks)", "messaging",
                        "SMS reminder via the messaging adapter; only after booking."));
    list.push_back(task("GPConsultation", "GP Consultation", "GP", "",
                        "Only after booking. Patient did not arrive? cancel() it (no-show)."));
    list.push_back(task("SpecialistReferral", "Specialist Referral", "Nurse", "",
                        "execute() sends the referral cost down Nurse -> GP -> Specialist -> clinical manager."));
    return list;
}

std::vector<ChainLink> HealthcareScenario::chain() const {
    std::vector<ChainLink> links;
    ChainLink a = { nurse->getRole(), nurse->getLimit(), false };
    ChainLink b = { gp->getRole(), gp->getLimit(), false };
    ChainLink c = { specialist->getRole(), specialist->getLimit(), false };
    ChainLink d = { clinicalManager->getRole(), clinicalManager->getLimit(), true };
    links.push_back(a);
    links.push_back(b);
    links.push_back(c);
    links.push_back(d);
    return links;
}

void HealthcareScenario::configureInstance(RuleContext& context) {
    ApprovalTask* referral = asApprovalTask(context.task("SpecialistReferral"));
    if (referral != nullptr) {
        std::string value = valueOr(context.data(), "referralValue", "0");
        referral->setRequestValue(std::atof(value.c_str()));
        context.trace().engine("ApprovalTask::setRequestValue(" + value + ")", "Chain of Responsibility",
                               "referral cost (R) travels down the chain");
    }
}

CaseData HealthcareScenario::connectorContext(RuleContext& context, const std::string& taskId) {
    CaseData data = context.data();
    data["task"] = taskId;
    if (taskId == "SendReminder") {
        data["message"] = "Reminder: " + data["name"] + ", your appointment with " +
                          valueOr(data, "resource.BookAppointment", "the clinic") + " is booked.";
    }
    return data;
}

Resource* HealthcareScenario::resourceFor(const std::string& taskId, ParticipantEntry& who) {
    return taskId == "BookAppointment" ? who.slots.get() : nullptr;
}

bool HealthcareScenario::canStart(RuleContext& context, const std::string& taskId, std::string& reason) {
    if (taskId != "BookAppointment") return true;
    if (!valueOr(context.data(), "resource.BookAppointment", "").empty()) return true;
    reason = "Booking cannot start: the ResourceCoordinator has not granted a doctor slot for this "
             "assignment. Escalate the task and reassign it to another GP.";
    return false;
}

void HealthcareScenario::afterTransition(RuleContext& context, const std::string& taskId, const std::string&,
                                         const std::string& newState, const std::string& outcome) {
    CaseData& data = context.data();
    Trace& trace = context.trace();

    if (taskId == "TriageAssessment" && newState == "Completed") {
        std::string urgency = outcome.empty() ? valueOr(data, "suggestedUrgency", "routine") : outcome;
        data["urgency"] = urgency;
        if (urgency == "urgent") {
            trace.scenario("HealthcareScenario routing rule", "routing (decision)",
                           "triage = URGENT -> emergency branch; booking branch cancelled");
            cancelByRule(context, "BookAppointment", "branch not taken");
            cancelByRule(context, "SendReminder", "branch not taken");
            cancelByRule(context, "GPConsultation", "branch not taken");
            cancelByRule(context, "SpecialistReferral", "branch not taken");
        } else {
            trace.scenario("HealthcareScenario routing rule", "routing (decision)",
                           "triage = ROUTINE -> booking branch; emergency branch cancelled");
            cancelByRule(context, "EmergencyConsultation", "branch not taken");
        }
    } else if (taskId == "EmergencyConsultation" && newState == "Completed") {
        data["outcome"] = "Treated by the doctor on duty";
    } else if (taskId == "BookAppointment" && newState == "Cancelled") {
        trace.scenario("HealthcareScenario rule", "exception path", "booking cancelled -> free the slot, close the visit");
        releaseByRule(context, "BookAppointment", "slot handed back");
        cancelByRule(context, "SendReminder", "no booking");
        cancelByRule(context, "GPConsultation", "no booking");
        cancelByRule(context, "SpecialistReferral", "no booking");
        data["outcome"] = "Booking cancelled";
    } else if (taskId == "GPConsultation" && newState == "Cancelled") {
        trace.scenario("HealthcareScenario rule", "exception path", "patient no-show -> free the slot, cancel follow-ups");
        releaseByRule(context, "BookAppointment", "no-show: slot handed back");
        cancelByRule(context, "SendReminder", "no-show");
        cancelByRule(context, "SpecialistReferral", "no-show");
        data["outcome"] = "Patient did not attend (no-show)";
    } else if (taskId == "GPConsultation" && newState == "Completed") {
        releaseByRule(context, "BookAppointment", "consultation done: slot handed back");
    } else if (taskId == "SpecialistReferral" && newState == "Completed") {
        data["outcome"] = "Referral approved: " + valueOr(data, "referral", "specialist");
    } else if (taskId == "SpecialistReferral" && newState == "Rejected") {
        data["outcome"] = "Referral declined (R" + valueOr(data, "referralValue", "?") + ")";
    }
}
