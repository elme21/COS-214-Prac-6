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
 *  - "scenario"  industry-specific code in web/scenarios/ (allowed by the spec)
 *  - "pending"   the designed class is not integrated yet, so nothing ran here
 */
struct TraceStep {
    std::string actor;
    std::string pattern;
    std::string detail;
    std::string kind;
};

class Trace {
public:
    void engine(const std::string& actor, const std::string& pattern, const std::string& detail) {
        add(actor, pattern, detail, "engine");
    }
    void temporary(const std::string& actor, const std::string& pattern, const std::string& detail) {
        add(actor, pattern, detail, "temporary");
    }
    void scenario(const std::string& actor, const std::string& pattern, const std::string& detail) {
        add(actor, pattern, detail, "scenario");
    }
    void pending(const std::string& actor, const std::string& pattern, const std::string& detail) {
        add(actor, pattern, detail, "pending");
    }

    const std::vector<TraceStep>& steps() const { return items; }

private:
    void add(const std::string& actor, const std::string& pattern,
             const std::string& detail, const std::string& kind) {
        TraceStep step;
        step.actor = actor;
        step.pattern = pattern;
        step.detail = detail;
        step.kind = kind;
        items.push_back(step);
    }

    std::vector<TraceStep> items;
};

#endif
