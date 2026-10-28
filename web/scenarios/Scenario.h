#ifndef WEB_SCENARIO_H
#define WEB_SCENARIO_H

#include "TaskLookup.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class DefinitionBuilder;
class Participant;
class Resource;
class ResourceCoordinator;
class WorkflowCommand;
class Staff;
class Trace;

typedef std::map<std::string, std::string> CaseData;

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
 * What an industry rule may touch in ONE running instance. Rules change tasks only by
 * running real engine commands through the instance's CommandProcessor.
 */
class RuleContext {
public:
    virtual ~RuleContext() {}
    virtual TaskRef task(const std::string& id) = 0;
    /** Runs a real command (taking ownership) through the instance's CommandProcessor. */
    virtual bool run(WorkflowCommand* command, const std::string& label) = 0;
    virtual CaseData& data() = 0;
    virtual Trace& trace() = 0;
    virtual ResourceCoordinator& coordinator() = 0;
    virtual Staff& staff() = 0;
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
    virtual std::string definitionName() const = 0;   // name in the DefinitionRegistry
    virtual std::vector<std::string> decoratedTasks() const = 0;

    /** Runs this industry's Director (and any extra builder steps) against the builder. */
    virtual void construct(DefinitionBuilder& builder) = 0;

    virtual void seedStaff(Staff& staff) = 0;
    virtual std::vector<SubjectSeed> subjects() const = 0;
    virtual std::vector<TaskInfo> tasks() const = 0;
    virtual std::vector<ChainLink> chain() const = 0;
    virtual std::string chainTaskId() const = 0;

    /** Called once per new instance (request values, which automated steps fail for this case). */
    virtual void configureInstance(RuleContext& context) = 0;

    /** Engine Resource that assigning this task to this person must request, or null. */
    virtual Resource* resourceFor(const std::string& taskId, const std::string& participantId, Staff& staff) = 0;

    /** Industry precondition for start(); returns false with a reason to refuse. */
    virtual bool canStart(RuleContext& context, const std::string& taskId, std::string& reason) = 0;

    /**
     * Routing and decision rules, run after an engine transition succeeded and before
     * the transition is published (so a cancelled branch is never unlocked).
     */
    virtual void afterTransition(RuleContext& context, const std::string& taskId,
                                 const std::string& newState, const std::string& outcome) = 0;
};

#endif
