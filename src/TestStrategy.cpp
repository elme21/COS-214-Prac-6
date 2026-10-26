#include "third_party/doctest.h"
#include <stdexcept>
#include <vector>
#include "TaskRouter.h"
#include "RoleBasedAssignment.h"
#include "WorkloadAssignment.h"
#include "RuleBasedAssignment.h"
#include "WorkflowEventBus.h"
#include "MonitoringCollector.h"
#include "DependencyActivator.h"
#include "WorkItem.h"
#include "Participant.h"
#include "WorkCoordinator.h"

namespace {
bool isAdmissions(WorkItem*, Participant* p) { return p->getRole() == "AdmissionsOfficer"; }
bool lightLoad(WorkItem*, Participant* p)    { return p->workload() < 5; }
}

TEST_SUITE("Strategy") {

struct Fixture {
    Participant admin1, admin2, fin;
    std::vector<Participant*> all;
    WorkItem assess;
    Fixture() : admin1("Thandi", "AdmissionsOfficer", 5),
                admin2("Pieter", "AdmissionsOfficer", 2),
                fin("Sipho", "FinanceOfficer", 0),
                assess("assess") {
        all.push_back(&admin1); all.push_back(&admin2); all.push_back(&fin);
    }
};

TEST_CASE_FIXTURE(Fixture, "RoleBasedAssignment assigns a participant holding the role") {
    RoleBasedAssignment s(all, "FinanceOfficer");
    s.route(&assess);
    CHECK(assess.getAssignee() == &fin);
}

TEST_CASE_FIXTURE(Fixture, "RoleBasedAssignment picks the first match and leaves item unassigned if none") {
    RoleBasedAssignment admins(all, "AdmissionsOfficer");
    admins.route(&assess);
    CHECK(assess.getAssignee() == &admin1);

    WorkItem other("other");
    RoleBasedAssignment nobody(all, "Registrar");
    nobody.route(&other);
    CHECK(other.getAssignee() == nullptr);
}

TEST_CASE_FIXTURE(Fixture, "WorkloadAssignment assigns the least loaded participant") {
    WorkloadAssignment s(all);
    s.route(&assess);
    CHECK(assess.getAssignee() == &fin);

    fin.setWorkload(9);
    WorkItem second("second");
    s.route(&second);
    CHECK(second.getAssignee() == &admin2);
}

TEST_CASE_FIXTURE(Fixture, "WorkloadAssignment with no candidates assigns nothing") {
    WorkloadAssignment s((std::vector<Participant*>()));
    s.route(&assess);
    CHECK(assess.getAssignee() == nullptr);
}

TEST_CASE_FIXTURE(Fixture, "RuleBasedAssignment requires every rule to hold") {
    RuleBasedAssignment s(all);
    s.addRule(isAdmissions);
    s.addRule(lightLoad);
    s.route(&assess);
    CHECK(assess.getAssignee() == &admin2);          // admin1 has load 5

    admin2.setWorkload(7);
    WorkItem second("second");
    s.route(&second);
    CHECK(second.getAssignee() == nullptr);          // nobody qualifies

    s.clearRules();
    s.route(&second);
    CHECK(second.getAssignee() == &admin1);          // no rules -> first candidate
}

TEST_CASE_FIXTURE(Fixture, "setCandidates changes who a strategy chooses from") {
    WorkloadAssignment s(all);
    std::vector<Participant*> onlyAdmins;
    onlyAdmins.push_back(&admin1);
    onlyAdmins.push_back(&admin2);
    s.setCandidates(onlyAdmins);
    s.route(&assess);
    CHECK(assess.getAssignee() == &admin2);
}

TEST_CASE_FIXTURE(Fixture, "TaskRouter delegates to the current strategy and can switch at runtime") {
    WorkloadAssignment byLoad(all);
    RoleBasedAssignment byRole(all, "AdmissionsOfficer");
    TaskRouter router(&byLoad);

    router.route(&assess);
    CHECK(assess.getAssignee() == &fin);

    router.setStrategy(&byRole);
    CHECK(router.getStrategy() == &byRole);
    router.route(&assess);
    CHECK(assess.getAssignee() == &admin1);
}

TEST_CASE_FIXTURE(Fixture, "TaskRouter rejects invalid use") {
    WorkloadAssignment s(all);

    SUBCASE("no strategy set") {
        TaskRouter router;
        CHECK_THROWS_AS(router.route(&assess), std::logic_error);
    }
    SUBCASE("null item") {
        TaskRouter router(&s);
        CHECK_THROWS_AS(router.route(nullptr), std::invalid_argument);
    }
    CHECK(assess.getAssignee() == nullptr);
}

TEST_CASE_FIXTURE(Fixture, "Observer and Strategy work together in one flow") {
    WorkflowEventBus bus;
    WorkCoordinator coord;
    MonitoringCollector mon(&bus);
    DependencyActivator dep(&bus, &coord);

    WorkItem registerModules("registerModules");
    dep.addWaitingItem(&registerModules);

    WorkloadAssignment s(all);
    TaskRouter router(&s);
    router.route(&assess);
    bus.publish(Event(EventType::ITEM_ASSIGNED, "app1", "assess"));   // done by WorkflowManager / WorkItem in the real system

    coord.markReady(&registerModules);
    bus.publish(Event(EventType::ITEM_COMPLETED, "app1", "assess"));

    CHECK(assess.getAssignee() == &fin);
    CHECK(registerModules.isAvailable());
    CHECK(mon.countOf(EventType::ITEM_ASSIGNED) == 1);
    CHECK(mon.countOf(EventType::ITEM_COMPLETED) == 1);
}

} // TEST_SUITE
