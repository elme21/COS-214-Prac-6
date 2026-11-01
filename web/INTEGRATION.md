# Integration status: what runs on the real engine

The web demo drives the team's real C++11 engine (`include/`, `src/`); it never contains a copy of it.
The engine is final. All 57 engine source files compile and link with `-std=c++11 -Wall -Wextra` and
**zero warnings**; `build.ps1` skips no file and needs no special flags.

The GUI is **fully generic**:
- Every scenario is a JSON **spec**: a file in `web/templates/`, or one made in the *+ New workflow* wizard.
- One `GenericDirector` turns any spec into a definition with the real `CodeDefinitionBuilder` and
  registers it in the real `DefinitionRegistry`.
- Nothing in `web/server`, `web/scenarios` or `web/public` mentions an industry.

## The Facade drives every case

`EngineGateway` is a thin JSON layer over the engine's Facade, `WorkflowManager`. One
`WorkflowManager` (with the registry and a `TaskRouter`) owns every case's tree, mediator and command
processor, as in the sequence diagrams: Client (browser) -> HTTP server -> `EngineGateway` ->
`WorkflowManager` -> engine.

| GUI action | WorkflowManager call |
|---|---|
| *Create case* | `createInstance(definitionName)` |
| *Start* | `start(instanceId)`: makes the ready tasks available and routes every task that has a strategy |
| Case view | `getStatus`, `getTree`, `getTaskIds`, `getTask`, `getItem`, `getCommands()->getHistory()` |
| *makeAvailable*, *assignTo*, *start*, *complete*, *approve*, *reject*, *cancel*, *escalate* | `makeAvailable`, `assign`, `start(instanceId, taskId)`, `complete`, `approve`, `reject`, `cancel`, `escalate` |
| Decision branches not taken, `cancel` effects | `cancel` |
| *Undo* | `undo` |
| *Restore* | `restoreSnapshot` |

What the Facade does not offer is a direct call on an object it hands out. Each is named in the
*Patterns in action* panel:

| Operation | Direct call | Why |
|---|---|---|
| *execute* | `execute()` (Template Method) on `getTask(...)` | `WorkflowManager` has no `execute`, and the engine has no command for it (not undoable) |
| *auto-assign* one task | `TaskRouter::route` on `getItem(...)`, with the same `TaskRouter` the Facade uses | `WorkflowManager` routes only inside `start()` |
| *Snapshot* | `WorkflowInstance::createSnapshot` on `getInstance(...)` | no snapshot operation (restore is `restoreSnapshot`) |
| Resource requests / releases | `ResourceCoordinator::requestResource` / `release` on `getMediator(...)` | no resource operation |
| Request value, automated-step result, case data | `ApprovalTask::setRequestValue`, `AutomatedTask::setStepSucceeds` on `getItem(...)`; `WorkflowInstance::setData` on `getInstance(...)` | no setters on the Facade |
| Events | a `WorkflowEventBus` per case, with `DependencyActivator`, `AuditLogger`, `NotificationSender`, `MonitoringCollector` | `WorkflowManager` publishes no events |
| An `unlock` effect on a task whose prerequisite will never complete (e.g. after a rejection) | `MakeAvailableCommand` run through `getCommands(...)` | `WorkflowManager::makeAvailable` refuses while an AND-prerequisite is incomplete; running it through the case's own processor keeps it undoable |

Refusals from `WorkflowManager` (it throws `std::logic_error` / `std::invalid_argument`) are shown as
normal refusal messages: for example "the case is not running (press Start first)" or "Dependency not
met".

The status shown in the GUI is `WorkflowManager::getStatus()`, except that a case shows **Finished** once
every task is completed, cancelled or rejected (`getStatus()` says "Completed" only if every task
completed, so a case with a cancelled branch stays "Running" in the engine).

---

## The 14 patterns in a normal GUI session

| Pattern | GUI action | Real classes that run | Status |
|---|---|---|---|
| Facade | Every case operation | `WorkflowManager` (table above) | Real; direct calls only where it has no operation |
| Builder | Server start (templates) or *Create workflow* in the wizard | `DefinitionBuilder`, `CodeDefinitionBuilder`, driven by `GenericDirector` | Real |
| Prototype | *Create case* | `WorkflowManager::createInstance` -> `DefinitionRegistry`, `WorkflowDefinition::cloneRoot`, `WorkComponent::clone` | Real |
| Composite | Every case view and action | `WorkStage`, `WorkItem` (nested sub-stages, `getChild`) from `WorkflowManager::getTree` | Real |
| Iterator | "Available work"; reading definitions | `ReadyTaskIterator` over `getTree()`; `DepthFirstIterator` (`createIterator`) for definitions | Real |
| Decorator | Any action on a task marked `"audit"` | `WorkComponentDecorator`, `AuditDecorator` (via `addDecorator`); `getTask` hands out the decorator | Real; the builder only offers `"audit"` |
| Template Method | *execute* | `WorkItem::execute`, `ManualTask` / `AutomatedTask` / `ApprovalTask::perform` | Real (direct call, see above) |
| State | Every action and refusal | `WorkItemState` and its 8 states | Real |
| Command | Every action button; *Undo* | `CommandProcessor` and the 8 concrete commands, run by `WorkflowManager` | Real; `execute` and auto-assign have no command |
| Memento | *Snapshot* / *Restore* | `WorkflowInstance`, `InstanceSnapshot`, `SnapshotHistory`; `WorkflowManager::restoreSnapshot` | Real |
| Mediator | *Start*, prerequisites, resource contention | `ResourceCoordinator` (registered by `WorkflowManager`), `Participant`, `Resource` | Real |
| Chain of Responsibility | *execute* on an approval task | `ApprovalHandler`, `RoleApproverHandler`, `FinalApproverHandler`, `ApprovalRequest`; result read from `ApprovalTask` | Real |
| Observer | Every state change | `WorkflowEventBus`, `Event`, `DependencyActivator`, `AuditLogger`, `NotificationSender`, `MonitoringCollector` | Real; the gateway publishes (the Facade publishes nothing). `DependencyActivator` unlocks dependants after `execute()`; after `WorkflowManager::complete()` the Facade has already notified the Mediator, and the panel says so |
| Strategy | *Start*; *auto-assign* | `TaskRouter`, `RoleBasedAssignment`, `WorkloadAssignment` | Real; the workload `WorkloadAssignment` reads is fed by a workaround (below) |

There are no references to the removed Adapter (or its connectors), `ChainProbe`, `TaskLookup` or the
old hard-coded scenarios anywhere in `web/`.

---

## What is still worked around, and why

Each workaround uses only real engine methods and is tagged **temporary web stand-in** in the panel.

### 1. `scenarios/StageNames`: stage names (Member 1)

`WorkStage` declares a private `name` member, but no constructor, setter or getter uses it, so the name
given to `CodeDefinitionBuilder::addStage` cannot be read back. `StageNames` matches each built stage to
the spec stage with the same depth and the same tasks beneath it. **Swap:** when `WorkStage` exposes its
name, read it in `EngineGateway::treeNode` and delete `StageNames`.

### 2. `EngineGateway::syncWorkload`: participant workload (Members 4 / 6)

`WorkloadAssignment` picks the participant with the lowest `Participant::workload()`, but no engine class
calls `itemAssigned()` / `itemReleased()`. The gateway calls those real methods when a task's holder
changes. **Swap:** when `AssignCommand` (or `WorkItem` / the Mediator) updates the workload, delete
`syncWorkload`.

### 3. Status after undoing the last completion (Member 7)

`WorkflowManager::complete()` sets the case status to "Completed" when every task is complete, and
`WorkflowManager::undo()` never sets it back. Every later action would then be refused with "Instance is
not running". After an undo, if the status is "Completed" but a task is open again, the gateway calls the
real `WorkflowInstance::setStatus("Running")`. **Swap:** when `undo()` (or `complete()`) keeps the status
consistent, delete those lines in `EngineGateway::undo`.

---

## Removed

| Removed | Because |
|---|---|
| `EngineGateway`'s own copy of the Facade's steps (own clone, mediator, command processor, wiring) | `WorkflowManager` now exposes a case's tree, tasks, mediator and command processor, and has every action |
| `scenarios/TaskLookup` | `WorkflowManager::getTaskIds` / `getTask` / `getItem`; reading a *definition* (before any case exists, outside the Facade) is the small display helper `scenarios/DefinitionTree` |
| Case-data copy beside each snapshot | `InstanceSnapshot` saves `WorkflowInstance::getData()` |
| `scenarios/ChainProbe`, `ChainTrace`, `StageNameRecorder`, `server/PendingIntegration` | Replaced by the real classes |
| `scenarios/EducationScenario`, `HealthcareScenario`, `HealthcareDirector`, `RuleHelpers`, `Staff`, `Scenario.h` | Industry code is now data: `web/templates/*.json` + `GenericDirector` + `ScenarioRuntime` |

---

## Engine behaviour the GUI shows as it is

| Behaviour | Owner | What the GUI does |
|---|---|---|
| `WorkflowManager::start()` assigns every ready task that has a strategy (e.g. *Submit Application* goes to the applicant) | Member 7 | Shown as routed by `WorkflowManager`; the task must be escalated before someone else can be assigned |
| A command's `undo()` restores only its own task: unlocked dependants, cancelled branches and taken resources stay | Member 5 | Shows the real result |
| `addDecorator` only knows `"audit"`; its sink is hard-wired to `std::cout` | Members 3/7 | Only "audit" is offered; the console is captured and the audit lines are shown |
| Approval handlers call `approve()` / `reject()` on the raw `ApprovalTask`, bypassing its decorators | Member 4 | Shown in the trace |
| `DefinitionRegistry::registerDefinition` takes a raw pointer; the builder returns a `shared_ptr` | Members 3/7 | Registers a copy (`new WorkflowDefinition(*built)`) |
| Dependencies are AND-only | Member 4 | Decisions cancel the branches not taken, so each branch must finish on its own (noted in the wizard) |
| `TaskRouter` strategies call `WorkItem::assignTo` directly, not through `AssignCommand` | Member 6 | Auto-assignment is labelled not undoable |
| `DeadlineEscalator` needs deadlines tracked and a clock event | Member 6 | Not used; escalation is the *escalate* button (`WorkflowManager::escalate`) |
