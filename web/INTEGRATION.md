# Temporary web stand-ins and how to remove them

The web demo must call the real C++ engine, never a hard-coded copy of it. Where a teammate's class
is missing, broken, or lacks an interface, a **temporary stand-in** in `web/` does the smallest
possible job, **using only real engine methods**. No stand-in reimplements state transitions,
cloning, dependency checks or approval-chain decisions.

Every stand-in file starts with:

```
// TEMPORARY - stands in for <class> (Member N) until <specific fix>. Delete when that lands.
```

In the UI, every step done by a stand-in is labelled **"(temporary web stand-in)"** in the
*Patterns in action* panel. Steps whose designed class does not exist yet are labelled
**"not yet integrated"**.

> **Member numbers** are taken from the comments in the code (`Member 1` in iterator files,
> `Member 3` for Prototype/Builder, `Member 4` for Participant/approval chain,
> `Member 7` for the Facade/Adapter layer). Command/Memento are assumed to be Member 5.
> Correct them if your team split differs.

All engine access goes through **one class, `web/server/EngineGateway`**. Its public methods mirror
`WorkflowManager` (`createInstance`, `start`, `complete`, `getStatus`) plus the other actions.
`main.cpp` and the browser code never touch the engine directly, so every swap below changes only
`EngineGateway.cpp` internals, `build.ps1`, or the stand-in file itself.

---

## 1. `server/EngineGateway` internals: stands in for `WorkflowManager` (Facade, Member 7)

**Why:** every `WorkflowManager` method throws `"waiting for integration"`, and the constructor
needs a non-null `TaskRouter*`, which does not exist yet.

**What it does today:** `createInstance` → `WorkflowDefinition::cloneRoot()` + `new WorkflowInstance`;
`start` → `ResourceCoordinator::canBecomeAvailable` + `makeAvailable()` on each task;
`complete` and the other actions → the work item's own lifecycle method inside a command.

**Swap when `WorkflowManager` works:**
1. Give `EngineGateway` a `WorkflowManager` member, built with the real `DefinitionRegistry` and `TaskRouter`.
2. In `EngineGateway::createInstance/start/complete/getStatus`, call the facade instead of the
   direct steps. Keep the JSON building and the trace lines (change their `kind` from
   `pending`/`temporary` to `engine`).
3. The facade needs a way to look up an instance and a task by ID (instance ID + task ID).
   `WorkItem::getId()` (Member 2) already exists for this.
4. `main.cpp` and `public/` stay unchanged.

## 2. `server/WebCommands`: stands in for `AssignCommand`, `CompleteCommand`, `ApproveCommand`, `RejectCommand`, `CancelCommand` (Command, Member 5)

**Why:** the five commands do not compile:
- `previousState = item->getState();` assigns `const WorkItemState*` to `std::string`.
- `item->restoreState(previousState);` uses one argument, but the contract takes `(state, assignee)`.
- `AssignCommand` calls `item->setAssignee(string)`, which does not exist on `WorkComponent`.

`build.ps1` therefore leaves them out (`$ExcludedEngineFiles`).

**What it does today:** `WebCommand : WorkflowCommand` is run by the **real** `CommandProcessor`.
`execute()` saves `(getState(), getAssignee())` of every task, then runs one user action (real
engine calls only). `undo()` calls `WorkComponent::restoreState(state, assignee)` on every task.
This is the back door the shared contract reserves for undo. It then calls a hook that puts case
data and slot allocations back.

**Swap:**
1. Fix the five commands, then remove their names from `$ExcludedEngineFiles` in `build.ps1`.
2. In `EngineGateway::act`, replace `new WebCommand(action, ...)` with the matching real command,
   e.g. `new CompleteCommand(item)`.
3. **Decide undo scope as a team.** A real `CompleteCommand` restores one item. A completion can also
   cancel a branch (routing rule) and unlock dependants (Mediator). A per-item undo leaves those
   effects in place. Either the commands store more state, or undo stays per-item and the demo
   says so.
4. Delete `WebCommands.h/.cpp`.

## 3. `server/HistoryLog`: stands in for a readable `CommandProcessor` history (Member 5) and `AuditLogger` (Observer, Member 6)

**Why:** `CommandProcessor` keeps its `stack` private, and `run()` returns `void`. `AuditLogger` does not exist.

**Swap:** when `AuditLogger` records events, or `CommandProcessor` exposes its history, have
`EngineGateway::instanceView` read the history from there. Then remove the `inst.log.add(...)` calls
and delete `HistoryLog.h/.cpp`.

## 4. `server/PendingIntegration`: the only wiring point for Observer and Strategy (Member 6)

**Why:** `EventPublisher`, `EventListener`, `DependencyActivator`, `AuditLogger`, `TaskRouter` and
`AssignmentStrategy` do not exist yet.

**What it does today:**
- `publishTransition()` is called after every successful transition. When an item becomes
  complete, it calls the real `Colleague::changed("completed")`. That makes the real
  `ResourceCoordinator::notifyChanged` unlock dependants. This is the job `DependencyActivator`
  will do.
- `chooseParticipant()` returns the participant picked in the UI.

**Swap:**
1. Add the Member 6 classes to `include/` and `src/` (the build picks up new `src/*.cpp` automatically).
2. Replace the bodies of the two functions with `eventPublisher->publish(...)` and
   `taskRouter->route(item, candidates)`, as the `TODO(Member 6)` comments say.
3. Register `DependencyActivator` and `AuditLogger` as listeners in `EngineGateway::build()`.
4. Delete the stand-in lines. The two call sites in `EngineGateway` stay.

## 5. `scenarios/RecordingDefinitionBuilder`: stands in for `CodeDefinitionBuilder::addDependency / setApprovalChain / addDecorator / setAssignmentStrategy` (Builder, Member 3)

**Why:** those four steps throw `"not integrated yet"`, so `EnrolmentDirector::construct` would throw.

**What it does today:** it is a second `DefinitionBuilder`. `addStage`/`addTask` go straight to the
real `CodeDefinitionBuilder`. Dependencies and chains are recorded. Then:
- the dependencies are given to the real `ResourceCoordinator::addDependency` for each instance;
- the chain is set with the real `ApprovalTask::setFirstHandler`;
- names and IDs are set with Member 2's `WorkItem::setName/setId`.

**Swap:**
1. When `CodeDefinitionBuilder` stores dependencies and chains (and `WorkflowDefinition` exposes
   them), use `CodeDefinitionBuilder` directly in `EducationScenario/HealthcareScenario::buildDefinition`.
2. Replace `wire()` in `EngineGateway` with whatever the definition/facade provides to register
   an instance's dependencies.
3. Delete `RecordingDefinitionBuilder.h/.cpp`.

## 6. `scenarios/NameLookup`: stands in for stage names (Member 1) and typed task access (shared contract)

**Why:** `WorkStage` has no name. `WorkComponent` has no `getName()/getId()` and no typed child
accessor, so the web layer uses `dynamic_cast` to reach `WorkItem`, `AutomatedTask` and `ApprovalTask`.

**What it does today:** stage names are keyed by **tree position** (path of child indices). This
works for every deep clone. `findTask()` and `allTasks()` use the tree's own `createIterator()`.

**Swap:** when `WorkStage` gets a name, read it in `EngineGateway::treeNode` and drop
`NameLookup::stageName`. If the team adds a lookup-by-ID to `WorkComponent` or `WorkflowInstance`,
use it in `findTask` and drop the casts.

## 7. `scenarios/ParticipantDirectory`: stands in for a name and role on `Participant` (Member 4)

**Why:** `Participant` only has `workload()`, so the UI could not show who a task is assigned to.

**Swap:** when `Participant` has `getName()/getRole()`, read those in `EngineGateway::treeNode`.
Keep the seeding code, which is industry data, and drop the name/role fields here.

## 8. `scenarios/TestConnectors` + `temporary_stubs/`: stand in for `Connector`, `ConnectorManager`, `PaymentConnector`, `MessagingConnector`, `StudentRecordsConnector` (Adapter, Member 7)

**Why:** none of the Adapter classes exist in `include/`/`src/`. `AutomatedTask.cpp` includes
`Connector.h` and `ConnectorManager.h`, which exist only as stubs (see section 13).

**What it does today:** `build.ps1` adds `-I web/temporary_stubs` (after `-I include`, so real
headers always win) and compiles the two stub `.cpp` files. `TestPaymentConnector` (medical-aid
pre-authorisation), `TestMessagingConnector` (SMS) and `TestStudentRecordsConnector` (ID check,
NSC results) wrap fake external systems. The spec allows this: "external services may be
represented by documented test adapters or stubs". The stub `ConnectorRequest` carries no data,
so the gateway calls `setContext(caseData)` on the adapter just before `AutomatedTask::execute()`.

**Swap:**
1. Add the real classes to `include/`/`src/`.
2. Follow section 13 to drop `web/temporary_stubs/`.
3. In `EngineGateway::build()`, register the real adapters with the real `ConnectorManager`. Pass
   case data through the real `ConnectorRequest` instead of `setContext`.
4. Delete `TestConnectors.h/.cpp`.

## 9. `scenarios/ChainTrace`: stands in for approval events (Observer, Member 6)

**Why:** handlers do not report which one handled a request. The demo must show escalation along the chain.

**What it does today:** `TracingRoleApproverHandler`/`TracingFinalApproverHandler` subclass the
real handlers. Each reports "request reached me" and then calls the **real** `handleRequest`, which
makes the decision.

**Swap:** when the chain publishes events (or `ApprovalRequest` records its path), use plain
`RoleApproverHandler`/`FinalApproverHandler` in the scenarios. Then delete `ChainTrace.h/.cpp`.

## 10. Definition kept in the gateway: stands in for `DefinitionRegistry` (Prototype manager, Member 3)

**Why:** `DefinitionRegistry.h` has no `.cpp`, so it would not link.

**Swap:** when `DefinitionRegistry.cpp` exists, `registerDefinition()` each built definition in
`EngineGateway::build()`. Look it up with `get(name)` in `createInstance`, or let `WorkflowManager` do it.

## 11. Case data next to snapshots: stands in for `WorkflowInstance::data` accessors (Member 5)

**Why:** `WorkflowInstance` has a private `data` map with no getter or setter, so per-instance
case data (urgency, decision, slot holder) lives in `EngineGateway::Instance::data`. A copy is
kept beside each `InstanceSnapshot`.

**Swap:** when `WorkflowInstance` exposes `getData()/setData()`, store the case data there. The
snapshot then carries it automatically.

## 12. Build flags (not code, approved)

- `-include cstddef` for engine files: `src/WorkStage.cpp` uses `size_t` without `<cstddef>` (Member 1).
  Remove the flag once the include is added.
- `-I <project root>`: `src/WorkflowManager.cpp` includes `"include/WorkflowManager.h"`. Remove once it
  includes `"WorkflowManager.h"`.

## 13. `temporary_stubs/`: copies of the `Connector` / `ConnectorManager` test stubs (Adapter, Member 7)

**Why:** the web build must work from the repository alone (`include/`, `src/`, `web/`). The only
files it needed from outside those folders were four test stubs in the local-only `tests/stubs/`:
`Connector.h`, `Connector.cpp`, `ConnectorManager.h` and `ConnectorManager.cpp`.
`src/AutomatedTask.cpp` includes the two headers, and nothing in `include/` provides them.

**What it is:** byte-for-byte copies of those four files. The only change is a two-line TEMPORARY
header at the top. `tests/stubs/` itself was not touched. The stubs for `ApprovalHandler`,
`Participant`, `WorkCoordinator` and `WorkIterator` were **not** copied, because the build uses
the real headers in `include/`.

**How `build.ps1` uses it:**
- `-I web/temporary_stubs` comes after `-I include`, so a real header with the same name always wins.
- Both `.cpp` files are compiled.
- Every path is derived from `$PSScriptRoot`, so no absolute path is involved.

**Swap when Member 7's real `Connector` and `ConnectorManager` exist in `include/`/`src/`:**
1. Delete `web/temporary_stubs/`.
2. In `build.ps1`, remove `$StubDir`, the `$stubs` line, `'-I', $StubDir`, `$StubDir` in `$headerDirs`,
   and the `foreach ($f in $stubs)` line.
3. Adjust `scenarios/TestConnectors` (section 8) to the real interface:
   `ConnectorRequest` will probably carry data, and `ConnectorManager::registerConnector` may be
   named differently.

## Not stood in for (shown honestly as gaps)

| Gap | Owner | What the demo does |
|---|---|---|
| `ReadyTaskIterator::isReadyItem` returns **Completed** items | Member 1 | Its real output is shown under each instance. "Available work" uses `DepthFirstIterator`. |
| `DepthFirstIterator`/`createIterator()` need `reset()` before first use | Member 1 | The web layer calls `reset()`. Consider pushing the root in the constructor. |
| `Resource` capacity never consumed (`isFree()` = `capacity > 0`) | Member 4 | Always calls the real `requestResource`/`release` and shows what the engine says. A GP with capacity 1 can currently be booked twice. Contention appears with no web change once capacity is real. |
| Concrete decorators have no `.cpp` | Decorator owner | Not applied. Recorded as "not yet integrated". |
| No routing/XOR mechanism in the engine | Members 3/6 | The routing rule lives in scenario code; the engine `cancel()`s the branch not taken. |
| `ApprovalRequest` value always 0 | Member 2 (**fixed**) | `ApprovalTask::setRequestValue` now carries a real value. |
