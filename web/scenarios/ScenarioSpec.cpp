#include "ScenarioSpec.h"

#include <cctype>
#include <set>
#include <sstream>

using nlohmann::json;

namespace {

/** Collects errors with a readable location prefix. */
class Errors {
public:
    explicit Errors(std::vector<std::string>& out) : list(out) {}
    void add(const std::string& where, const std::string& what) { list.push_back(where + ": " + what); }
    bool any() const { return !list.empty(); }
private:
    std::vector<std::string>& list;
};

std::string asText(const json& v) {
    if (v.is_string()) return v.get<std::string>();
    if (v.is_number_integer()) return std::to_string(v.get<long long>());
    if (v.is_number()) {
        std::ostringstream out;
        out << v.get<double>();
        return out.str();
    }
    if (v.is_boolean()) return v.get<bool>() ? "true" : "false";
    return "";
}

std::string str(const json& j, const char* key, const std::string& fallback = std::string()) {
    json::const_iterator it = j.find(key);
    if (it == j.end() || it->is_null()) return fallback;
    std::string s = asText(*it);
    return s.empty() ? fallback : s;
}

std::vector<std::string> strList(const json& j, const char* key) {
    std::vector<std::string> out;
    json::const_iterator it = j.find(key);
    if (it == j.end() || !it->is_array()) return out;
    for (size_t i = 0; i < it->size(); ++i) {
        std::string s = asText((*it)[i]);
        if (!s.empty()) out.push_back(s);
    }
    return out;
}

const json& arr(const json& j, const char* key) {
    static const json empty = json::array();
    json::const_iterator it = j.find(key);
    return it != j.end() && it->is_array() ? *it : empty;
}

bool validId(const std::string& id) {
    if (id.empty() || id.size() > 60) return false;
    if (!std::isalnum(static_cast<unsigned char>(id[0]))) return false;   // "_..." is reserved for the root stage
    for (size_t i = 0; i < id.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(id[i]);
        if (!std::isalnum(c) && c != '_') return false;
    }
    return true;
}

bool isNumber(const std::string& s, double& value) {
    if (s.empty()) return false;
    std::istringstream in(s);
    in >> value;
    return !in.fail() && in.eof();
}

bool validOp(const std::string& op) {
    return op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=" || op == "contains";
}

std::string where(const std::string& kind, size_t index, const std::string& id) {
    std::ostringstream out;
    out << kind << "[" << index << "]";
    if (!id.empty()) out << " '" << id << "'";
    return out.str();
}

RuleSpec readRule(const json& j) {
    RuleSpec r;
    if (!j.is_object()) return r;
    r.field = str(j, "field");
    r.op = str(j, "op", "==");
    r.value = str(j, "value");
    r.branch = str(j, "branch");
    r.present = !r.field.empty();
    return r;
}

EffectSpec readEffect(const json& j, const char* key) {
    EffectSpec e;
    json::const_iterator it = j.find(key);
    if (it == j.end() || !it->is_object()) return e;
    json::const_iterator set = it->find("set");
    if (set != it->end() && set->is_object()) {
        for (json::const_iterator s = set->begin(); s != set->end(); ++s) e.set[s.key()] = asText(s.value());
    }
    e.outcome = str(*it, "outcome");
    e.release = strList(*it, "release");
    e.cancel = strList(*it, "cancel");
    e.unlock = strList(*it, "unlock");
    return e;
}

void readStages(const json& list, std::vector<StageSpec>& out, Errors& errors, const std::string& path) {
    for (size_t i = 0; i < list.size(); ++i) {
        const json& s = list[i];
        StageSpec stage;
        if (!s.is_object()) { errors.add(path + where("stages", i, ""), "must be an object"); continue; }
        stage.id = str(s, "id");
        stage.name = str(s, "name", stage.id);
        std::string here = path + where("stages", i, stage.id);
        readStages(arr(s, "stages"), stage.stages, errors, here + ".");
        const json& tasks = arr(s, "tasks");
        for (size_t k = 0; k < tasks.size(); ++k) {
            const json& t = tasks[k];
            TaskSpec task;
            if (!t.is_object()) { errors.add(here + "." + where("tasks", k, ""), "must be an object"); continue; }
            task.id = str(t, "id");
            task.name = str(t, "name", task.id);
            task.type = str(t, "type", "manual");
            for (size_t c = 0; c < task.type.size(); ++c) task.type[c] = static_cast<char>(std::tolower(task.type[c]));
            task.role = str(t, "role");
            task.help = str(t, "help");
            task.stageId = stage.id;
            task.dependsOn = strList(t, "dependsOn");
            task.resource = str(t, "resource");
            task.decorators = strList(t, "decorators");
            task.strategy = str(t, "strategy", "none");
            json::const_iterator ok = t.find("succeeds");
            task.succeeds = ok == t.end() || !ok->is_boolean() || ok->get<bool>();
            json::const_iterator rule = t.find("successRule");
            if (rule != t.end()) task.successRule = readRule(*rule);
            json::const_iterator rv = t.find("requestValue");
            if (rv != t.end()) {
                if (rv->is_object()) {
                    task.requestField = str(*rv, "field");
                    double v = 0;
                    if (isNumber(str(*rv, "value"), v)) task.requestValue = v;
                } else {
                    double v = 0;
                    if (isNumber(asText(*rv), v)) task.requestValue = v;
                }
            }
            const json& chain = arr(t, "chain");
            for (size_t c = 0; c < chain.size(); ++c) {
                ChainLevelSpec level;
                level.label = str(chain[c], "label");
                level.role = str(chain[c], "role");
                double limit = 0;
                if (!isNumber(str(chain[c], "limit"), limit)) {
                    errors.add(here + "." + where("tasks", k, task.id) + ".chain[" + std::to_string(c) + "]",
                               "limit must be a number");
                }
                level.limit = limit;
                task.chain.push_back(level);
            }
            task.onComplete = readEffect(t, "onComplete");
            task.onReject = readEffect(t, "onReject");
            task.onCancel = readEffect(t, "onCancel");
            stage.tasks.push_back(task);
        }
        out.push_back(stage);
    }
}

void collectTasks(const std::vector<StageSpec>& stages, std::vector<const TaskSpec*>& out) {
    for (size_t i = 0; i < stages.size(); ++i) {
        collectTasks(stages[i].stages, out);   // sub-stages first is fine: order only matters for display
        for (size_t k = 0; k < stages[i].tasks.size(); ++k) out.push_back(&stages[i].tasks[k]);
    }
}

void collectStageIds(const std::vector<StageSpec>& stages, std::vector<std::string>& out) {
    for (size_t i = 0; i < stages.size(); ++i) {
        out.push_back(stages[i].id);
        collectStageIds(stages[i].stages, out);
    }
}

bool stageHasWork(const StageSpec& s) {
    if (!s.tasks.empty()) return true;
    for (size_t i = 0; i < s.stages.size(); ++i) if (stageHasWork(s.stages[i])) return true;
    return false;
}

void checkStagesNotEmpty(const std::vector<StageSpec>& stages, Errors& errors) {
    for (size_t i = 0; i < stages.size(); ++i) {
        if (!stageHasWork(stages[i])) errors.add("stage '" + stages[i].id + "'", "has no tasks (directly or in a sub-stage)");
        checkStagesNotEmpty(stages[i].stages, errors);
    }
}

// Depth-first cycle search over dependsOn. Returns the cycle as "A -> B -> A", or "".
std::string findCycle(const ScenarioSpec& spec) {
    std::map<std::string, int> mark;   // 0 new, 1 on path, 2 done
    std::vector<std::string> path;
    std::vector<const TaskSpec*> tasks = spec.allTasks();
    struct Walker {
        const ScenarioSpec& spec;
        std::map<std::string, int>& mark;
        std::vector<std::string>& path;
        std::string visit(const std::string& id) {
            mark[id] = 1;
            path.push_back(id);
            const TaskSpec* t = spec.task(id);
            if (t != nullptr) {
                for (size_t i = 0; i < t->dependsOn.size(); ++i) {
                    const std::string& next = t->dependsOn[i];
                    if (spec.task(next) == nullptr) continue;
                    if (mark[next] == 1) {
                        std::string cycle;
                        bool on = false;
                        for (size_t p = 0; p < path.size(); ++p) {
                            if (path[p] == next) on = true;
                            if (on) cycle += path[p] + " -> ";
                        }
                        return cycle + next;
                    }
                    if (mark[next] == 0) {
                        std::string found = visit(next);
                        if (!found.empty()) return found;
                    }
                }
            }
            path.pop_back();
            mark[id] = 2;
            return "";
        }
    } walker = { spec, mark, path };
    for (size_t i = 0; i < tasks.size(); ++i) {
        if (mark[tasks[i]->id] == 0) {
            std::string found = walker.visit(tasks[i]->id);
            if (!found.empty()) return found;
        }
    }
    return "";
}

std::string defaultPrefix(const std::string& name) {
    std::string prefix;
    bool start = true;
    for (size_t i = 0; i < name.size() && prefix.size() < 4; ++i) {
        unsigned char c = static_cast<unsigned char>(name[i]);
        if (std::isalnum(c)) {
            if (start) prefix += static_cast<char>(std::toupper(c));
            start = false;
        } else {
            start = true;
        }
    }
    return prefix.empty() ? "WF" : prefix;
}

} // namespace

// ---------------------------------------------------------------------------
// Look-ups
// ---------------------------------------------------------------------------

std::vector<const TaskSpec*> ScenarioSpec::allTasks() const {
    std::vector<const TaskSpec*> out;
    collectTasks(stages, out);
    return out;
}

const TaskSpec* ScenarioSpec::task(const std::string& id) const {
    std::vector<const TaskSpec*> tasks = allTasks();
    for (size_t i = 0; i < tasks.size(); ++i) if (tasks[i]->id == id) return tasks[i];
    return nullptr;
}

const InputSpec* ScenarioSpec::input(const std::string& key) const {
    for (size_t i = 0; i < inputs.size(); ++i) if (inputs[i].key == key) return &inputs[i];
    return nullptr;
}

const ParticipantSpec* ScenarioSpec::participant(const std::string& id) const {
    for (size_t i = 0; i < participants.size(); ++i) if (participants[i].id == id) return &participants[i];
    return nullptr;
}

const ResourceSpec* ScenarioSpec::resource(const std::string& id) const {
    for (size_t i = 0; i < resources.size(); ++i) if (resources[i].id == id) return &resources[i];
    return nullptr;
}

const DecisionSpec* ScenarioSpec::decisionAfter(const std::string& taskId) const {
    for (size_t i = 0; i < decisions.size(); ++i) if (decisions[i].after == taskId) return &decisions[i];
    return nullptr;
}

// ---------------------------------------------------------------------------
// Parse + validate
// ---------------------------------------------------------------------------

bool parseScenarioSpec(const json& j, ScenarioSpec& spec, std::vector<std::string>& out) {
    Errors errors(out);
    if (!j.is_object()) {
        errors.add("spec", "must be a JSON object");
        return false;
    }
    spec = ScenarioSpec();
    spec.source = j;

    // --- read ---------------------------------------------------------------
    spec.name = str(j, "name");
    spec.industry = str(j, "industry");
    spec.description = str(j, "description");
    spec.idPrefix = str(j, "idPrefix");
    spec.subjectField = str(j, "subjectField");

    const json& inputs = arr(j, "inputs");
    for (size_t i = 0; i < inputs.size(); ++i) {
        InputSpec in;
        in.key = str(inputs[i], "key");
        in.label = str(inputs[i], "label", in.key);
        in.type = str(inputs[i], "type", "text");
        in.options = strList(inputs[i], "options");
        spec.inputs.push_back(in);
    }
    const json& people = arr(j, "participants");
    for (size_t i = 0; i < people.size(); ++i) {
        ParticipantSpec p;
        p.id = str(people[i], "id");
        p.name = str(people[i], "name", p.id);
        p.role = str(people[i], "role");
        spec.participants.push_back(p);
    }
    const json& resources = arr(j, "resources");
    for (size_t i = 0; i < resources.size(); ++i) {
        ResourceSpec r;
        r.id = str(resources[i], "id");
        r.name = str(resources[i], "name", r.id);
        double capacity = -1;
        if (!isNumber(str(resources[i], "capacity"), capacity) || capacity < 0 || capacity != static_cast<int>(capacity)) {
            errors.add(where("resources", i, r.id), "capacity must be a whole number >= 0");
        }
        r.capacity = capacity < 0 ? 0 : static_cast<int>(capacity);
        r.owner = str(resources[i], "owner");
        spec.resources.push_back(r);
    }
    readStages(arr(j, "stages"), spec.stages, errors, "");
    const json& decisions = arr(j, "decisions");
    for (size_t i = 0; i < decisions.size(); ++i) {
        const json& d = decisions[i];
        DecisionSpec dec;
        dec.id = str(d, "id");
        dec.label = str(d, "label", dec.id);
        dec.after = str(d, "after");
        dec.mode = str(d, "mode", "manual");
        const json& branches = arr(d, "branches");
        for (size_t b = 0; b < branches.size(); ++b) {
            BranchSpec branch;
            branch.id = str(branches[b], "id");
            branch.label = str(branches[b], "label", branch.id);
            branch.tasks = strList(branches[b], "tasks");
            dec.branches.push_back(branch);
        }
        const json& rules = arr(d, "rules");
        for (size_t r = 0; r < rules.size(); ++r) dec.rules.push_back(readRule(rules[r]));
        dec.defaultBranch = str(d, "default", dec.branches.empty() ? std::string() : dec.branches[0].id);
        spec.decisions.push_back(dec);
    }
    const json& seeds = arr(j, "seeds");
    for (size_t i = 0; i < seeds.size(); ++i) {
        SeedSpec seed;
        seed.label = str(seeds[i], "label");
        seed.summary = str(seeds[i], "summary");
        json::const_iterator data = seeds[i].find("data");
        if (data != seeds[i].end() && data->is_object()) {
            for (json::const_iterator it = data->begin(); it != data->end(); ++it) seed.data[it.key()] = asText(it.value());
        }
        spec.seeds.push_back(seed);
    }

    // --- validate -----------------------------------------------------------
    if (spec.name.empty()) errors.add("name", "is required");
    if (spec.industry.empty()) errors.add("industry", "is required");
    if (spec.idPrefix.empty()) spec.idPrefix = defaultPrefix(spec.name);
    for (size_t i = 0; i < spec.idPrefix.size(); ++i) {
        if (!std::isalnum(static_cast<unsigned char>(spec.idPrefix[i]))) {
            errors.add("idPrefix", "may only contain letters and digits");
            break;
        }
    }

    std::set<std::string> keys;
    for (size_t i = 0; i < spec.inputs.size(); ++i) {
        const InputSpec& in = spec.inputs[i];
        std::string w = where("inputs", i, in.key);
        if (!validId(in.key)) errors.add(w, "key must start with a letter or digit and use only letters, digits or _");
        else if (!keys.insert(in.key).second) errors.add(w, "duplicate key");
        if (in.type != "text" && in.type != "number" && in.type != "choice") errors.add(w, "type must be text, number or choice");
        if (in.type == "choice" && in.options.size() < 2) errors.add(w, "a choice needs at least two options");
    }
    if (spec.subjectField.empty()) {
        for (size_t i = 0; i < spec.inputs.size() && spec.subjectField.empty(); ++i) {
            if (spec.inputs[i].type == "text") spec.subjectField = spec.inputs[i].key;
        }
    } else if (spec.input(spec.subjectField) == nullptr) {
        errors.add("subjectField", "'" + spec.subjectField + "' is not an input field");
    }

    std::set<std::string> roles;
    std::set<std::string> people_ids;
    if (spec.participants.empty()) errors.add("participants", "add at least one participant");
    for (size_t i = 0; i < spec.participants.size(); ++i) {
        const ParticipantSpec& p = spec.participants[i];
        std::string w = where("participants", i, p.id);
        if (!validId(p.id)) errors.add(w, "id must start with a letter or digit and use only letters, digits or _");
        else if (!people_ids.insert(p.id).second) errors.add(w, "duplicate id");
        if (p.role.empty()) errors.add(w, "role is required");
        else roles.insert(p.role);
    }

    std::set<std::string> resource_ids;
    bool ownedResource = false;
    for (size_t i = 0; i < spec.resources.size(); ++i) {
        const ResourceSpec& r = spec.resources[i];
        std::string w = where("resources", i, r.id);
        if (!validId(r.id)) errors.add(w, "id must start with a letter or digit and use only letters, digits or _");
        else if (!resource_ids.insert(r.id).second) errors.add(w, "duplicate id");
        if (!r.owner.empty()) {
            if (spec.participant(r.owner) == nullptr) errors.add(w, "owner '" + r.owner + "' is not a participant");
            ownedResource = true;
        }
    }

    // Stage and task ids share one namespace (the builder rejects duplicates across both).
    std::set<std::string> names;
    std::vector<std::string> stageIds;
    collectStageIds(spec.stages, stageIds);
    if (spec.stages.empty()) errors.add("stages", "add at least one stage");
    for (size_t i = 0; i < stageIds.size(); ++i) {
        if (!validId(stageIds[i])) errors.add("stage '" + stageIds[i] + "'", "id must start with a letter or digit and use only letters, digits or _");
        else if (!names.insert(stageIds[i]).second) errors.add("stage '" + stageIds[i] + "'", "duplicate id");
    }
    checkStagesNotEmpty(spec.stages, errors);

    std::vector<const TaskSpec*> tasks = spec.allTasks();
    for (size_t i = 0; i < tasks.size(); ++i) {
        const TaskSpec& t = *tasks[i];
        std::string w = "task '" + t.id + "'";
        if (!validId(t.id)) { errors.add("task '" + t.id + "'", "id must start with a letter or digit and use only letters, digits or _"); continue; }
        if (!names.insert(t.id).second) errors.add(w, "duplicate id (stage and task ids must all be different)");
        if (t.type != "manual" && t.type != "automated" && t.type != "approval") errors.add(w, "type must be manual, automated or approval");
        if (t.role.empty()) errors.add(w, "responsible role is required");
        else if (!roles.count(t.role)) errors.add(w, "no participant has the role '" + t.role + "'");
        for (size_t d = 0; d < t.dependsOn.size(); ++d) {
            if (t.dependsOn[d] == t.id) errors.add(w, "cannot depend on itself");
            else if (spec.task(t.dependsOn[d]) == nullptr) errors.add(w, "depends on unknown task '" + t.dependsOn[d] + "'");
        }
        if (t.resource == "@assignee") {
            if (!ownedResource) errors.add(w, "uses the assignee's resource, but no resource has an owner");
        } else if (!t.resource.empty() && spec.resource(t.resource) == nullptr) {
            errors.add(w, "unknown resource '" + t.resource + "'");
        }
        for (size_t d = 0; d < t.decorators.size(); ++d) {
            if (t.decorators[d] != "audit") {
                errors.add(w, "decorator '" + t.decorators[d] + "' is not offered by CodeDefinitionBuilder::addDecorator (only 'audit')");
            }
        }
        if (t.strategy != "none" && t.strategy != "role" && t.strategy != "workload") errors.add(w, "strategy must be none, role or workload");
        if (t.successRule.present) {
            if (t.type != "automated") errors.add(w, "only automated tasks have a success rule");
            if (spec.input(t.successRule.field) == nullptr) errors.add(w, "success rule uses unknown input '" + t.successRule.field + "'");
            if (!validOp(t.successRule.op)) errors.add(w, "success rule operator '" + t.successRule.op + "' is not supported");
        }
        if (t.type == "approval") {
            if (t.chain.empty()) errors.add(w, "an approval task needs an approval chain with at least one level");
            if (!t.requestField.empty()) {
                const InputSpec* in = spec.input(t.requestField);
                if (in == nullptr) errors.add(w, "request value uses unknown input '" + t.requestField + "'");
                else if (in->type != "number") errors.add(w, "request value input '" + t.requestField + "' must be a number field");
            }
        } else if (!t.chain.empty()) {
            errors.add(w, "only approval tasks have an approval chain");
        }
        const EffectSpec* effects[] = { &t.onComplete, &t.onReject, &t.onCancel };
        const char* effectNames[] = { "onComplete", "onReject", "onCancel" };
        for (int e = 0; e < 3; ++e) {
            const std::vector<std::string>* lists[] = { &effects[e]->release, &effects[e]->cancel, &effects[e]->unlock };
            for (int l = 0; l < 3; ++l) {
                for (size_t x = 0; x < lists[l]->size(); ++x) {
                    if (spec.task((*lists[l])[x]) == nullptr) {
                        errors.add(w, std::string(effectNames[e]) + " refers to unknown task '" + (*lists[l])[x] + "'");
                    }
                }
            }
        }
    }
    if (tasks.empty()) errors.add("stages", "add at least one task");

    std::string cycle = findCycle(spec);
    if (!cycle.empty()) errors.add("dependencies", "cycle " + cycle + " (a task would wait for itself)");

    std::set<std::string> decisionIds;
    std::set<std::string> decisionAfters;
    for (size_t i = 0; i < spec.decisions.size(); ++i) {
        const DecisionSpec& d = spec.decisions[i];
        std::string w = where("decisions", i, d.id);
        if (!validId(d.id)) errors.add(w, "id must start with a letter or digit and use only letters, digits or _");
        else if (!decisionIds.insert(d.id).second || keys.count(d.id)) errors.add(w, "id clashes with another decision or an input field");
        if (spec.task(d.after) == nullptr) errors.add(w, "'after' must name a task (the decision is made when it completes)");
        else if (!decisionAfters.insert(d.after).second) errors.add(w, "another decision already follows task '" + d.after + "'");
        if (d.mode != "manual" && d.mode != "rule") errors.add(w, "mode must be manual or rule");
        if (d.branches.size() < 2) errors.add(w, "needs at least two branches");
        std::set<std::string> branchIds;
        std::set<std::string> branchTasks;
        for (size_t b = 0; b < d.branches.size(); ++b) {
            const BranchSpec& br = d.branches[b];
            std::string bw = w + "." + where("branches", b, br.id);
            if (!validId(br.id)) errors.add(bw, "id must start with a letter or digit and use only letters, digits or _");
            else if (!branchIds.insert(br.id).second) errors.add(bw, "duplicate branch id");
            if (br.tasks.empty()) errors.add(bw, "a branch needs at least one task");
            for (size_t t = 0; t < br.tasks.size(); ++t) {
                if (spec.task(br.tasks[t]) == nullptr) errors.add(bw, "unknown task '" + br.tasks[t] + "'");
                else if (br.tasks[t] == d.after) errors.add(bw, "cannot contain the task the decision follows");
                else if (!branchTasks.insert(br.tasks[t]).second) errors.add(bw, "task '" + br.tasks[t] + "' is in two branches");
            }
        }
        for (size_t r = 0; r < d.rules.size(); ++r) {
            const RuleSpec& rule = d.rules[r];
            std::string rw = w + ".rules[" + std::to_string(r) + "]";
            if (spec.input(rule.field) == nullptr) errors.add(rw, "unknown input '" + rule.field + "'");
            if (!validOp(rule.op)) errors.add(rw, "operator '" + rule.op + "' is not supported");
            if (!branchIds.count(rule.branch)) errors.add(rw, "unknown branch '" + rule.branch + "'");
        }
        if (!d.defaultBranch.empty() && !branchIds.count(d.defaultBranch)) errors.add(w, "default branch '" + d.defaultBranch + "' does not exist");
        if (d.mode == "rule" && d.rules.empty()) errors.add(w, "a rule-based decision needs at least one rule");
    }

    for (size_t i = 0; i < spec.seeds.size(); ++i) {
        const SeedSpec& s = spec.seeds[i];
        std::string w = where("seeds", i, s.label);
        if (s.label.empty()) errors.add(w, "label is required");
        for (CaseData::const_iterator it = s.data.begin(); it != s.data.end(); ++it) {
            const InputSpec* in = spec.input(it->first);
            double n = 0;
            if (in == nullptr) errors.add(w, "'" + it->first + "' is not an input field");
            else if (in->type == "number" && !it->second.empty() && !isNumber(it->second, n)) errors.add(w, "'" + it->first + "' must be a number");
        }
    }
    return !errors.any();
}
