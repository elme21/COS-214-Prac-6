#ifndef WEB_TRACE_H
#define WEB_TRACE_H

#include <string>
#include <vector>

/**
 * @brief One step in the "Patterns in action" panel.
 *
 * Every step is recorded at the point in the code where the call actually
 * happens, so the panel is a log of what ran, not a description of the design.
 *
 * kind:
 *  - "engine"    a real TIVIDY engine class (include/, src/) did this step
 *  - "temporary" a web stand-in did this step (see web/INTEGRATION.md)
 *  - "scenario"  a rule from the scenario spec decided this step (data, not engine code)
 *  - "pending"   the engine class exists but the GUI cannot use it, so nothing ran here
 */
struct TraceStep {
    std::string actor;     ///< The class/method (or stand-in) that acted
    std::string pattern;   ///< The design pattern it belongs to
    std::string detail;    ///< What happened
    std::string kind;      ///< engine, temporary, scenario or pending
};

/// @brief The list of steps recorded during one gateway call.
class Trace {
public:
    /// @brief Records a step done by a real engine class.
    /// @param actor the class/method that acted
    /// @param pattern the design pattern
    /// @param detail what happened
    void engine(const std::string& actor, const std::string& pattern, const std::string& detail) {
        add(actor, pattern, detail, "engine");
    }
    /// @brief Records a step done by a web stand-in.
    /// @param actor the stand-in that acted
    /// @param pattern the design pattern
    /// @param detail what happened and why the engine could not do it
    void temporary(const std::string& actor, const std::string& pattern, const std::string& detail) {
        add(actor, pattern, detail, "temporary");
    }
    /// @brief Records a step decided by a rule from the scenario spec.
    /// @param actor the rule
    /// @param pattern the design pattern it feeds
    /// @param detail what the rule decided
    void scenario(const std::string& actor, const std::string& pattern, const std::string& detail) {
        add(actor, pattern, detail, "scenario");
    }
    /// @brief Records an engine class that exists but could not be used, so nothing ran.
    /// @param actor the class/method that was not used
    /// @param pattern the design pattern
    /// @param detail why it could not be used
    void pending(const std::string& actor, const std::string& pattern, const std::string& detail) {
        add(actor, pattern, detail, "pending");
    }

    /// @brief Gets the recorded steps.
    /// @return the steps in the order they were recorded
    const std::vector<TraceStep>& steps() const { return items; }

private:
    /// @brief Appends one step.
    /// @param actor the actor
    /// @param pattern the design pattern
    /// @param detail what happened
    /// @param kind engine, temporary, scenario or pending
    void add(const std::string& actor, const std::string& pattern,
             const std::string& detail, const std::string& kind) {
        TraceStep step;
        step.actor = actor;
        step.pattern = pattern;
        step.detail = detail;
        step.kind = kind;
        items.push_back(step);
    }

    std::vector<TraceStep> items;   ///< The recorded steps
};

#endif
