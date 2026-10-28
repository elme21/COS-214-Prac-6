#include "HealthcareDirector.h"

#include "DefinitionBuilder.h"

HealthcareDirector::HealthcareDirector(ApprovalHandler* chain) : referralChain(chain) {}

void HealthcareDirector::construct(DefinitionBuilder& builder)
{
    // Root stage
    builder.addStage("ClinicVisit", "");

    // Stage 1: intake at reception
    builder.addStage("Intake", "ClinicVisit");
    builder.addTask("RegisterPatient", "manual", "Intake");
    builder.addTask("VerifyMedicalAid", "automated", "Intake");

    // Stage 2: triage decides urgency (routing point)
    builder.addStage("Triage", "ClinicVisit");
    builder.addTask("TriageAssessment", "manual", "Triage");

    // Urgent branch
    builder.addStage("EmergencyCare", "ClinicVisit");
    builder.addTask("EmergencyConsultation", "manual", "EmergencyCare");

    // Routine branch: compete for a limited doctor slot, then remind the patient
    builder.addStage("Booking", "ClinicVisit");
    builder.addTask("BookAppointment", "manual", "Booking");
    builder.addTask("SendReminder", "automated", "Booking");

    // Stage 4: consultation and referral approval
    builder.addStage("Consultation", "ClinicVisit");
    builder.addTask("GPConsultation", "manual", "Consultation");
    builder.addTask("SpecialistReferral", "approval", "Consultation");

    // dependencies
    builder.addDependency("RegisterPatient", "VerifyMedicalAid");
    builder.addDependency("VerifyMedicalAid", "TriageAssessment");

    // both branches wait for triage; the routing rule cancels the branch not taken
    builder.addDependency("TriageAssessment", "EmergencyConsultation");
    builder.addDependency("TriageAssessment", "BookAppointment");

    // no reminder and no consultation before a slot is booked
    builder.addDependency("BookAppointment", "SendReminder");
    builder.addDependency("BookAppointment", "GPConsultation");

    // a referral follows the GP consultation
    builder.addDependency("GPConsultation", "SpecialistReferral");

    builder.setApprovalChain("SpecialistReferral", referralChain);

    // every attempt on the referral (including refused ones) is audited
    builder.addDecorator("SpecialistReferral", "audit");
}
