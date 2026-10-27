// TEMPORARY - stands in for PaymentConnector, MessagingConnector and StudentRecordsConnector
// (Member 7) until the real Adapter classes and the real Connector / ConnectorManager /
// ConnectorRequest exist in include/. Delete when that lands.
//
// Compiled against web/temporary_stubs/Connector.h and ConnectorManager.h (copies of the test stubs), which
// define the interface AutomatedTask already calls. The spec allows external services
// to be "represented by documented test adapters or stubs".
#ifndef WEB_TESTCONNECTORS_H
#define WEB_TESTCONNECTORS_H

#include "Connector.h"

#include <map>
#include <string>

typedef std::map<std::string, std::string> CaseData;

// ---------------------------------------------------------------------------
// Adaptees: pretend external systems with their own, non-TIVIDY interfaces.
// ---------------------------------------------------------------------------

/** A medical-aid switch (like the ones SA practices use to pre-authorise claims). */
class FakeMedicalAidSwitch {
public:
    /** Returns an authorisation code, or "" when the membership is not active. */
    std::string preAuthorise(const std::string& scheme, const std::string& memberNumber,
                             const std::string& status) const;
};

/** A bulk SMS gateway. */
class FakeSmsGateway {
public:
    bool sendSms(const std::string& cellphone, const std::string& text, std::string& messageId) const;
};

/** Home Affairs ID check plus the matric (NSC) results database. */
class FakeStudentRecordsService {
public:
    bool idNumberValid(const std::string& idNumber) const;
    bool nscResultsFound(const std::string& examNumber) const;
};

// ---------------------------------------------------------------------------
// Adapters: present the one Connector interface the engine expects.
// ---------------------------------------------------------------------------

/**
 * Base for the test adapters. The stub ConnectorRequest carries no data, so the
 * gateway hands the case data to the adapter just before AutomatedTask::execute().
 */
class TestConnector : public Connector {
public:
    void setContext(const CaseData& data) { context = data; }
    const std::string& lastResult() const { return result; }
    virtual std::string adapteeName() const = 0;

protected:
    std::string field(const std::string& key) const;
    CaseData context;
    std::string result;
};

/** PaymentConnector stand-in: medical-aid authorisation for the clinic. */
class TestPaymentConnector : public TestConnector {
public:
    ConnectorResult execute(const ConnectorRequest& request) override;
    std::string adapteeName() const override { return "FakeMedicalAidSwitch::preAuthorise"; }

private:
    FakeMedicalAidSwitch adaptee;
};

/** MessagingConnector stand-in: SMS reminders and applicant notifications. */
class TestMessagingConnector : public TestConnector {
public:
    ConnectorResult execute(const ConnectorRequest& request) override;
    std::string adapteeName() const override { return "FakeSmsGateway::sendSms"; }

private:
    FakeSmsGateway adaptee;
};

/** StudentRecordsConnector stand-in: ID verification and NSC results lookup. */
class TestStudentRecordsConnector : public TestConnector {
public:
    ConnectorResult execute(const ConnectorRequest& request) override;
    std::string adapteeName() const override { return "FakeStudentRecordsService"; }

private:
    FakeStudentRecordsService adaptee;
};

#endif
