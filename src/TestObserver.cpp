#include "third_party/doctest.h"
#include <ctime>
#include <string>
#include "WorkflowEventBus.h"
#include "DependencyActivator.h"
#include "AuditLogger.h"
#include "NotificationSender.h"
#include "MonitoringCollector.h"
#include "DeadlineEscalator.h"
#include "WorkItem.h"
#include "WorkCoordinator.h"

TEST_SUITE("Observer") {

TEST_CASE("Bus has no state until something is published") {
    WorkflowEventBus bus;
    CHECK(bus.getState() == nullptr);
    bus.setState(Event(EventType::ITEM_COMPLETED, "app1", "assess"));
    REQUIRE(bus.getState() != nullptr);
    CHECK(bus.getState()->itemId == "assess");
}

TEST_CASE("Listeners pull the new state via getState() when updated") {
    WorkflowEventBus bus;
    MonitoringCollector mon(&bus);
    bus.publish(Event(EventType::ITEM_COMPLETED, "app1", "assess"));
    CHECK(mon.countOf(EventType::ITEM_COMPLETED) == 1);
}

TEST_CASE("Duplicate subscription is ignored") {
    WorkflowEventBus bus;
    MonitoringCollector mon(&bus);
    bus.subscribe(EventType::ITEM_COMPLETED, &mon);
    CHECK(bus.listenerCount(EventType::ITEM_COMPLETED) == 1);
    bus.publish(Event(EventType::ITEM_COMPLETED, "app1", "x"));
    CHECK(mon.countOf(EventType::ITEM_COMPLETED) == 1);
}

TEST_CASE("Unsubscribe stops delivery") {
    WorkflowEventBus bus;
    MonitoringCollector mon(&bus);
    bus.unsubscribe(EventType::ITEM_COMPLETED, &mon);
    bus.publish(Event(EventType::ITEM_COMPLETED, "app1", "x"));
    CHECK(mon.countOf(EventType::ITEM_COMPLETED) == 0);
}

TEST_CASE("Publishing with no listeners is harmless") {
    WorkflowEventBus bus;
    CHECK_NOTHROW(bus.publish(Event(EventType::INSTANCE_CREATED, "app1")));
}

TEST_CASE("Listener destructor unsubscribes itself from the bus") {
    WorkflowEventBus bus;
    {
        AuditLogger log(&bus);
        CHECK(bus.listenerCount(EventType::ITEM_COMPLETED) == 1);
    }
    CHECK(bus.listenerCount(EventType::ITEM_COMPLETED) == 0);
    CHECK_NOTHROW(bus.publish(Event(EventType::ITEM_COMPLETED, "a", "b")));
}

TEST_CASE("Separate buses are isolated (instance isolation)") {
    WorkflowEventBus busA, busB;
    MonitoringCollector monA(&busA), monB(&busB);
    busA.publish(Event(EventType::ITEM_COMPLETED, "appA", "step1"));
    CHECK(monA.totalEvents() == 1);
    CHECK(monB.totalEvents() == 0);
    CHECK(busB.getState() == nullptr);
}

TEST_CASE("AuditLogger records every event with details") {
    WorkflowEventBus bus;
    AuditLogger audit(&bus);
    bus.publish(Event(EventType::INSTANCE_CREATED, "app1"));
    bus.publish(Event(EventType::EXTERNAL_EVENT, "app1", "", "payment received"));

    REQUIRE(audit.getLog().size() == 2);
    CHECK(audit.getLog()[0].find("INSTANCE_CREATED") != std::string::npos);
    CHECK(audit.getLog()[1].find("payment received") != std::string::npos);
}

TEST_CASE("NotificationSender only reports events the applicant cares about") {
    WorkflowEventBus bus;
    NotificationSender notify(&bus);
    bus.publish(Event(EventType::INSTANCE_CREATED, "app1"));
    bus.publish(Event(EventType::ITEM_COMPLETED, "app1", "verify"));
    bus.publish(Event(EventType::EXTERNAL_EVENT, "app1", "", "payment received"));

    REQUIRE(notify.getSent().size() == 2);
    CHECK(notify.getSent()[0].find("verify") != std::string::npos);
    CHECK(notify.getSent()[1].find("payment received") != std::string::npos);
}

TEST_CASE("MonitoringCollector counts per type and per instance") {
    WorkflowEventBus bus;
    MonitoringCollector mon(&bus);
    bus.publish(Event(EventType::ITEM_COMPLETED, "app1", "a"));
    bus.publish(Event(EventType::ITEM_COMPLETED, "app1", "b"));
    bus.publish(Event(EventType::ITEM_COMPLETED, "app2", "a"));
    bus.publish(Event(EventType::ITEM_REJECTED, "app2", "b"));

    CHECK(mon.totalEvents() == 4);
    CHECK(mon.countOf(EventType::ITEM_COMPLETED) == 3);
    CHECK(mon.completedFor("app1") == 2);
    CHECK(mon.completedFor("app2") == 1);
    CHECK(mon.completedFor("nobody") == 0);
    CHECK(mon.report().find("ITEM_REJECTED: 1") != std::string::npos);
}

TEST_CASE("DependencyActivator only activates items the coordinator approves") {
    WorkflowEventBus bus;
    WorkCoordinator coord;
    DependencyActivator dep(&bus, &coord);

    WorkItem fees("payFees"), modules("selectModules");
    dep.addWaitingItem(&fees);
    dep.addWaitingItem(&modules);
    dep.addWaitingItem(&modules);                 // duplicate ignored
    CHECK(dep.waitingCount() == 2);

    SUBCASE("nothing ready -> nothing activated") {
        bus.publish(Event(EventType::ITEM_COMPLETED, "app1", "offerAccepted"));
        CHECK_FALSE(fees.isAvailable());
        CHECK_FALSE(modules.isAvailable());
        CHECK(dep.waitingCount() == 2);
    }
    SUBCASE("only the ready item is activated") {
        coord.markReady(&fees);
        bus.publish(Event(EventType::EXTERNAL_EVENT, "app1", "", "offer accepted"));
        CHECK(fees.isAvailable());
        CHECK_FALSE(modules.isAvailable());
        CHECK(dep.waitingCount() == 1);
    }
    SUBCASE("irrelevant events do not trigger activation") {
        coord.markReady(&fees);
        bus.publish(Event(EventType::ITEM_ASSIGNED, "app1", "x"));
        CHECK_FALSE(fees.isAvailable());
    }
}

TEST_CASE("DeadlineEscalator escalates overdue items exactly once") {
    WorkflowEventBus bus;
    DeadlineEscalator esc(&bus);
    int calls = 0;
    esc.setEscalationAction([&calls](const Event&) { ++calls; });

    std::time_t now = std::time(nullptr);
    esc.trackDeadline("review", "app1", now + 100);
    esc.trackDeadline("hodSignOff", "app1", now + 500);

    CHECK(esc.checkDeadlines(now) == 0);
    CHECK(calls == 0);

    CHECK(esc.checkDeadlines(now + 200) == 1);
    CHECK(calls == 1);
    REQUIRE(esc.getEscalated().size() == 1);
    CHECK(esc.getEscalated()[0] == "review");

    CHECK(esc.checkDeadlines(now + 200) == 0);
    CHECK(calls == 1);
}

TEST_CASE("DeadlineEscalator drops deadlines of finished items and auto-tracks available ones") {
    WorkflowEventBus bus;
    DeadlineEscalator esc(&bus);
    esc.setDefaultTimeout(60);

    bus.publish(Event(EventType::ITEM_AVAILABLE, "app1", "assess"));
    CHECK(esc.trackedCount() == 1);
    bus.publish(Event(EventType::ITEM_CANCELLED, "app1", "assess"));
    CHECK(esc.trackedCount() == 0);
}

} // TEST_SUITE
