// TEMPORARY - stands in for PaymentConnector, MessagingConnector and StudentRecordsConnector
// (Member 7) until the real Adapter classes and the real Connector / ConnectorManager /
// ConnectorRequest exist in include/. Delete when that lands.
#include "TestConnectors.h"

#include <cctype>

// --- adaptees ---------------------------------------------------------------

std::string FakeMedicalAidSwitch::preAuthorise(const std::string& scheme, const std::string& memberNumber,
                                               const std::string& status) const {
    if (scheme.empty() || memberNumber.empty() || status != "active") return "";
    return "AUTH-" + memberNumber.substr(memberNumber.size() > 4 ? memberNumber.size() - 4 : 0);
}

bool FakeSmsGateway::sendSms(const std::string& cellphone, const std::string& text,
                             std::string& messageId) const {
    if (cellphone.size() < 10 || text.empty()) return false;
    messageId = "SMS-" + cellphone.substr(cellphone.size() - 4);
    return true;
}

bool FakeStudentRecordsService::idNumberValid(const std::string& idNumber) const {
    // South African ID numbers: 13 digits with a Luhn check digit.
    if (idNumber.size() != 13) return false;
    int sum = 0;
    for (int i = 0; i < 13; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(idNumber[i]))) return false;
        int digit = idNumber[i] - '0';
        if (i % 2 == 1) {
            digit *= 2;
            if (digit > 9) digit -= 9;
        }
        sum += digit;
    }
    return sum % 10 == 0;
}

bool FakeStudentRecordsService::nscResultsFound(const std::string& examNumber) const {
    // Only National Senior Certificate exam numbers are in the results database.
    return examNumber.compare(0, 4, "NSC-") == 0;
}

// --- adapters ---------------------------------------------------------------

std::string TestConnector::field(const std::string& key) const {
    CaseData::const_iterator it = context.find(key);
    return it == context.end() ? std::string() : it->second;
}

ConnectorResult TestPaymentConnector::execute(const ConnectorRequest&) {
    std::string code = adaptee.preAuthorise(field("medicalAid"), field("memberNumber"), field("medicalAidStatus"));
    if (code.empty()) {
        result = "Medical aid declined: " + field("medicalAid") + " membership is " + field("medicalAidStatus");
        return ConnectorResult(false);
    }
    result = "Pre-authorised by " + field("medicalAid") + " (" + code + ")";
    return ConnectorResult(true);
}

ConnectorResult TestMessagingConnector::execute(const ConnectorRequest&) {
    std::string text = field("message");
    if (text.empty()) text = "TIVIDY update for " + field("name");
    std::string messageId;
    if (!adaptee.sendSms(field("cellphone"), text, messageId)) {
        result = "SMS gateway refused the message to " + field("cellphone");
        return ConnectorResult(false);
    }
    result = "SMS " + messageId + " to " + field("cellphone") + ": \"" + text + "\"";
    return ConnectorResult(true);
}

ConnectorResult TestStudentRecordsConnector::execute(const ConnectorRequest&) {
    if (field("task") == "VerifyResults") {
        if (!adaptee.nscResultsFound(field("examNumber"))) {
            result = "Results database has no record for " + field("examNumber") +
                     " (" + field("curriculum") + " is not an NSC curriculum)";
            return ConnectorResult(false);
        }
        result = "NSC results found for " + field("examNumber") + " (APS " + field("aps") + ")";
        return ConnectorResult(true);
    }
    if (!adaptee.idNumberValid(field("idNumber"))) {
        result = "ID number " + field("idNumber") + " failed the Home Affairs check";
        return ConnectorResult(false);
    }
    result = "ID number " + field("idNumber") + " verified";
    return ConnectorResult(true);
}
