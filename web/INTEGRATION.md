# Integration status: what runs on the real engine

The web demo drives the team's real C++11 engine (`include/`, `src/`); it never contains a copy of it.
The engine is final. All 57 engine source files compile and link with `-std=c++11 -Wall -Wextra` and
**zero warnings**; `build.ps1` skips no file and needs no special flags.

The GUI is **fully generic**:
- Every scenario is a JSON **spec**: a file in `web/templates/`, or one made in the *+ New workflow* wizard.
- One `GenericDirector` turns any spec into a definition with the real `CodeDefinitionBuilder` and
  registers it in the real `DefinitionRegistry`.
- Nothing in `web/server`, `web/scenarios` or `web/public` mentions an industry.

What the GUI cannot do through the engine is done by a few small **workarounds**. Each uses **only
real engine methods**: none reimplements state transitions, cloning, dependency checks, resource
counting, approval decisions or assignment rules. In the *Patterns in action* panel a workaround step
is tagged **temporary web stand-in**, and an engine class the GUI cannot use is tagged **not used**.

Members: 1 Composite + Iterator, 2 State + Template Method, 3 Decorator + Prototype,
4 Mediator + Chain, 5 Command + Memento, 6 Observer + Strategy, 7 Builder + Facade.

---

## The 14 patterns in a normal GUI session

| Pattern | GUI action | Real classes that run | Status |
|---|---|---|---|
| Builder | Server start (templates) or *Create workflow* in the wizard | `DefinitionBuilder`, `CodeDefinitionBuilder` (`addStage`, `addTask`, `addDependency`, `setApprovalChain`, `setAssignmentStrategy`, `addDecorator`), driven by `GenericDirector` | Real |
| Prototype | *Create case* | `DefinitionRegistry`, `WorkflowDefinition::cloneRoot`, `WorkComponent::clone` | Real |
| Composite | Every case view; every action | `WorkStage`, `WorkItem` (nested sub-stages, `getChild`) | Real |
| Iterator | Every case view; *Available work* | `DepthFirstIterator` (`createIterator`), `ReadyTaskIterator` | Real |
| Decorator | Any action on a task marked `"audit"` | `WorkComponentDecorator`, `AuditDecorator` (via `addDecorator`) | Real; the builder only offers `"audit"` |
| Template Method | *execute* | `WorkItem::execute`, `ManualTask` / `AutomatedTask` / `ApprovalTask::perform`, `AutomatedTask::setStepSucceeds` | Real |
| State | Every action, including every refusal | `WorkItemState` and its 8 concrete states | Real |
| Command | Every action button; *Undo* | `CommandProcessor` (`run`, `undo`, `getHistory`) and the 8 concrete commands | Real; `execute` and auto-assign have no command, so they are not undoable |
| Memento | *Snapshot* / *Restore* | `WorkflowInstance::createSnapshot` / `restoreSnapshot`, `InstanceSnapshot`, `SnapshotHistory` | Real: status, case data and tree are saved and restored |
| Mediator | *Start*, *makeAvailable*, assigning a task that needs a resource | `ResourceCoordinator` (`addDependency`, `canBecomeAvailable`, `requestResource`, `release`), `Participant`, `Resource` | Real |
| Chain of Responsibility | *execute* on an approval task | `ApprovalHandler`, `RoleApproverHandler`, `FinalApproverHandler`, `ApprovalRequest`; the result is read from `ApprovalTask::getChainPath` / `getChainOutcome` / `getChainDecidedBy` | Real |
| Observer | Every state change | `WorkflowEventBus`, `Event`, `DependencyActivator` (unlocks dependants through the Mediator), `AuditLogger`, `NotificationSender`, `MonitoringCollector` | Real; `EngineGateway` publishes the events |
| Strategy | *auto-assign* | `TaskRouter`, `RoleBasedAssignment`, `WorkloadAssignment` | Real; the workload that `WorkloadAssignment` reads is fed by a workaround (below) |
| Facade | (none) | `WorkflowManager` | **Not usable by a GUI** (below); `EngineGateway` does the same steps with the real classes |

There are no references to the removed Adapter (or its connectors), to `ChainProbe`, or to the old
hard-coded scenarios anywhere in `web/`.

---

## What is still worked around, and why

### 1. `server/EngineGateway` does the Facade's job (Member 7)

`WorkflowManager` compiles and links, and its `createInstance`, `start` and `getStatus` work. But a
GUI cannot use it for a case it shows:
- `createInstance()` returns only an id. The instance's tree, mediator and command processor live in a
  private `Runtime`, with no accessor, so the case cannot be shown or acted on.
- It offers only `complete()`, and `complete()` requires the task to be `InProgress`. No Facade
  operation can assign or start a task, so `complete()` always throws "Task must be InProgress before
  completion". (Checked with a small program on the real classes: create, start, then complete the
  first task.)
- It publishes no events, so the Observer listeners would never run.

Routing a GUI case through it would mean acting on a second, hidden copy, so the GUI does not use it.
`EngineGateway` mirrors its interface (`createInstance`, `start`, `complete`, `getStatus`) and does the
same steps with the same real classes: clone from `DefinitionRegistry`, a per-instance
`ResourceCoordinator` and `CommandProcessor`, dependencies from `getDependencies()`, the real commands,
the real `TaskRouter`, and real `Event`s on a per-instance `WorkflowEventBus`. Every action records
`WorkflowManager::<operation>` as **not used** in the *Patterns in action* panel, with this reason.

**Swap:** give `WorkflowManager` accessors (instance, task by id, command processor, event bus) or the
missing operations, then make `EngineGateway` hold a `WorkflowManager` and call it.

### 2. `scenarios/TaskLookup`: finding a task by id (Member 7)

`WorkflowManager` keeps its `components` / `items` maps private. `TaskLookup` walks the tree with
`createIterator()` and unwraps decorators with `getWrappedComponent()`, as
`WorkflowManager::createInstance` does internally. **Swap:** use the Facade's lookup once it exists.

### 3. `scenarios/StageNames`: stage names (Member 1)

`WorkStage` now declares a private `name` member, but no constructor, setter or getter uses it, so the
name given to `CodeDefinitionBuilder::addStage` cannot be read back. `StageNames` matches each built
stage to the spec stage with the same depth and the same tasks beneath it. **Swap:** when `WorkStage`
exposes its name, read it in `EngineGateway::treeNode` and delete `StageNames`.

### 4. `EngineGateway::syncWorkload`: participant workload (Members 4 / 6)

`WorkloadAssignment` picks the participant with the lowest `Participant::workload()`, but no engine
class calls `itemAssigned()` / `itemReleased()`. The gateway calls those real methods when a task's
holder changes. **Swap:** when `AssignCommand` (or `WorkItem` / the Mediator) updates the workload,
delete `syncWorkload`.

---

## Stand-ins removed

| Removed | Because |
|---|---|
| Case-data copy beside each snapshot | `InstanceSnapshot` now saves `WorkflowInstance::getData()`, and `restoreSnapshot` restores it |
| `scenarios/ChainProbe` | `ApprovalTask` keeps the result of its last chain run (`getChainPath`, `getChainOutcome`, `getChainDecidedBy`) |
| Copies of the final approver's name kept for `FinalApproverHandler` | It now takes `const std::string&` and the base class stores a copy |
| `scenarios/ChainTrace`, `scenarios/StageNameRecorder`, `server/PendingIntegration` | Replaced earlier by the real classes |
| `scenarios/EducationScenario`, `HealthcareScenario`, `HealthcareDirector`, `RuleHelpers`, `Staff`, `Scenario.h` | Industry code is now data: `web/templates/*.json` + `GenericDirector` + `ScenarioRuntime` |
| The `/api/meta` endpoint | Nothing used it |

---

## Engine behaviour the GUI shows as it is

| Behaviour | Owner | What the GUI does |
|---|---|---|
| A command's `undo()` restores only its own task: unlocked dependants, cancelled branches and taken resources stay | Member 5 | Shows the real result; each rule-driven cancel is its own command, so it is undone separately |
| `addDecorator` only knows `"audit"`; its sink is hard-wired to `std::cout` | Members 3/7 | Only "audit" is offered; the console is captured and the audit lines are shown |
| Approval handlers call `approve()` / `reject()` on the raw `ApprovalTask`, bypassing its decorators | Member 4 | Shown in the trace |
| `DefinitionRegistry::registerDefinition` takes a raw pointer; the builder returns a `shared_ptr` | Members 3/7 | Registers a copy (`new WorkflowDefinition(*built)`) |
| Dependencies are AND-only (`canBecomeAvailable` needs every prerequisite complete) | Member 4 | Decisions cancel the branches not taken, so each branch must finish on its own (noted in the wizard) |
| `TaskRouter` strategies call `WorkItem::assignTo` directly, not through `AssignCommand` | Member 6 | Auto-assignment is labelled not undoable |
| `execute()` has no command | Members 2/5 | Called directly; labelled not undoable |
| `DeadlineEscalator` needs deadlines tracked and a clock event | Member 6 | Not used; escalation is the *escalate* button (`EscalateCommand`) |
