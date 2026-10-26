#ifndef WEB_SCENARIO_H
#define WEB_SCENARIO_H

#include "TestConnectors.h"   // CaseData

#include <memory>
#include <string>
#include <vector>

class WorkItem;
class WorkflowDefinition;
class Resource;
class ResourceCoordinator;
class RecordingDefinitionBuilder;
class ParticipantDirectory;
struct ParticipantEntry;
class Trace;

/** A seeded case: an applicant or a patient, with realistic input data. */
struct SubjectSeed {
    std::string id;
    std::string name;
    std::string summary;
    CaseData data;
};

/** Industry-specific facts about one task in the definition (display and routing input). */
struct TaskInfo {
    std::string id;
    std::string label;
    std::string roleHint;                 // who normally does it (Strategy is not integrated)
    std::string connector;                // ConnectorManager name for automated tasks
    std::string outcomeQuestion;          // asked when the task is completed, if any
    std::vector<std::string> outcomes;    // allowed answers
    std::string help;                     // one line for the marker
};

/** One link of an approval chain, for display. */
struct ChainLink {
    std::string role;
    double limit;
    bool isFinal;
};

/**
 * What an industry rule may touch in ONE running instance. Rules only call real
 * engine methods on these objects; the gateway records the effects.
 */
class RuleContext {
public:
    virtual ~RuleContext() {}
    virtual WorkItem* task(const std::string& id) = 0;
    virtual CaseData& data() = 0;
    virtual Trace& trace() = 0;
    virtual ResourceCoordinator& coordinator() = 0;
    virtual ParticipantDirectory& participants() = 0;
};

/**
 * Industry-specific definitions, rules and data (allowed by the spec, section 2).
 * The generic engine never sees this interface: only web/server/EngineGateway does.
 */
class Scenario {
public:
    virtual ~Scenario() {}

    virtual std::string key() const = 0;
    virtual std::string title() const = 0;
    virtual std::string description() const = 0;
    virtual std::string subjectNoun() const = 0;      // "applicant" / "patient"
    virtual std::string idPrefix() const = 0;         // "EDU" / "HC"
    virtual std::string directorName() const = 0;

    /** Runs this industry's Director against the builder (Builder pattern). */
    virtual std::shared_ptr<WorkflowDefinition> buildDefinition(RecordingDefinitionBuilder& builder) = 0;

    virtual void seedParticipants(ParticipantDirectory& directory) = 0;
    virtual std::vector<SubjectSeed> subjects() const = 0;
    virtual std::vector<TaskInfo> tasks() const = 0;
    virtual std::vector<ChainLink> chain() const = 0;
    virtual std::string chainTaskId() const = 0;

    /** Called once per new instance (e.g. set ApprovalTask request values from the case). */
    virtual void configureInstance(RuleContext& context) = 0;

    /** Data handed to a test connector before an automated task runs. */
    virtual CaseData connectorContext(RuleContext& context, const std::string& taskId) = 0;

    /** Engine Resource that assigning this task to this person must request, or null. */
    virtual Resource* resourceFor(const std::string& taskId, ParticipantEntry& who) = 0;

    /** Industry precondition for start(); returns false with a reason to refuse. */
    virtual bool canStart(RuleContext& context, const std::string& taskId, std::string& reason) = 0;

    /**
     * Routing and decision rules, run after an engine transition succeeded and
     * before dependants are notified. Rules call only engine methods (cancel(),
     * makeAvailable(), ResourceCoordinator::release(), ...).
     */
    virtual void afterTransition(RuleContext& context, const std::string& taskId, const std::string& action,
                                 const std::string& newState, const std::string& outcome) = 0;
};

#endif
