# Integration status: what is real, what is still a stand-in

The web demo calls the real C++ engine; it never contains a hard-coded copy of it. The engine code in
`include/` and `src/` is final. Every one of its 57 source files compiles and links with
`-std=c++11 -Wall -Wextra` and **zero warnings**. `build.ps1` skips no engine file and needs no
special flags.

The GUI is **fully generic**:
- Every scenario is a JSON **spec**: one of the files in `web/templates/`, or one created in the editor.
- One `GenericDirector` turns any spec into a definition with the real `CodeDefinitionBuilder`, and
  registers it in the real `DefinitionRegistry`.
- Nothing in `web/server`, `web/scenarios` or `web/public` mentions an industry.

A few small **temporary stand-ins** remain where an engine class is missing an accessor. Each one
uses **only real engine methods**: no stand-in reimplements state transitions, cloning, dependency
checks, resource counting, approval decisions or assignment rules. Every stand-in file starts with:

```
// TEMPORARY - stands in for <class> (Member N) until <specific fix>. Delete when that lands.
```

In the UI, a step done by a stand-in is labelled **"(temporary web stand-in)"** in the
*Patterns in action* panel, and the unusable Facade is labelled **"not used by the GUI"**.

Members: 1 Composite + Iterator, 2 State + Template Method, 3 Decorator + Prototype,
4 Mediator + Chain, 5 Command + Memento, 6 Observer + Strategy, 7 Builder + Facade.

---

## Real engine classes the GUI uses

| Pattern | Real classes | How the GUI uses them |
|---|---|---|
| Builder | `DefinitionBuilder`, `CodeDefinitionBuilder` | `GenericDirector` (web/scenarios) calls `addStage`, `addTask`, `addDependency`, `setApprovalChain`, `setAssignmentStrategy` and `addDecorator` for any spec. `EnrolmentDirector` is **no longer used**: its definition is now the `university-admission.json` template. |
| Prototype | `DefinitionRegistry`, `WorkflowDefinition::cloneRoot`, `WorkComponent::clone` | Every definition is registered; every instance is a deep clone. |
| Composite | `WorkStage`, `WorkItem` | Stages with nested sub-stages and tasks. |
| Iterator | `DepthFirstIterator`, `ReadyTaskIterator` | "Available work" is `ReadyTaskIterator`. |
| Decorator | `WorkComponentDecorator`, `AuditDecorator` | Tasks marked `"decorators": ["audit"]` are wrapped by `addDecorator`. |
| Template Method | `WorkItem::execute`, `ManualTask`, `AutomatedTask`, `ApprovalTask` | `execute`; automated steps succeed or fail via `setStepSucceeds` (from the spec's rule). |
| State | `WorkItemState` + 8 states | Every refusal comes from a state. |
| Command | `CommandProcessor` (+ `getHistory()`), the 8 concrete commands | Every action and every rule-driven cancel or unlock is a real command; Undo pops it. |
| Memento | `WorkflowInstance`, `InstanceSnapshot`, `SnapshotHistory` | Snapshot / Restore. Case data is stored with `WorkflowInstance::setData`. |
| Mediator | `ResourceCoordinator`, `Participant`, `Resource` | Dependencies per instance; live resource contention (`take`/`giveBack`). |
| Chain of Responsibility | `ApprovalHandler`, `RoleApproverHandler`, `FinalApproverHandler`, `ApprovalRequest` | One chain per approval task, with generic levels from the spec: "Approver level 1..N", "Final approver". |
| Observer | `WorkflowEventBus`, `DependencyActivator`, `AuditLogger`, `NotificationSender`, `MonitoringCollector` | One bus per instance; dependants are unlocked by `DependencyActivator`; the audit trail is `AuditLogger::getLog()`. |
| Strategy | `TaskRouter`, `RoleBasedAssignment`, `WorkloadAssignment` | "auto-assign" routes a task with the strategy its spec names; manual assign is always available. |

## Stand-ins removed this round

| Removed | Because |
|---|---|
| `server/PendingIntegration` (Strategy hook) | The Strategy classes now compile: `route()` is `bool` everywhere |
| `scenarios/ChainTrace` (tracing handler subclasses) | `handleRequest` is no longer virtual; it was first replaced by the smaller `ChainProbe` |
| `scenarios/ChainProbe` (pass-through links that reported the path) | `ApprovalTask` (Member 2) now keeps the result of its last chain run as plain data: `getChainPath()`, `getChainOutcome()`, `getChainDecidedBy()`, `wasChainDecided()`, `wasChainApproved()`, `hasChainResult()`. The gateway reads these after `execute()`, and the task JSON carries them as `chainResult` |
| `scenarios/StageNameRecorder` | Replaced by `StageNames`, which works for any spec |
| `scenarios/EducationScenario`, `HealthcareScenario`, `HealthcareDirector`, `RuleHelpers`, `Staff`, `Scenario.h` | Industry code is now data: `web/templates/*.json` + `GenericDirector` + `ScenarioRuntime` |
| `build.ps1` exclusions and `-include WorkComponent.h` | Every engine file now builds as-is |

---

## Remaining stand-ins

### 1. `server/EngineGateway` internals: stand in for `WorkflowManager` (Facade, Member 7)

**Why the Facade cannot be used by a GUI:** `WorkflowManager` now compiles and links, but:
- `createInstance()` returns only an id. The instance's tree, mediator and command processor live in
  a private `Runtime`, and there is no accessor, so the GUI cannot show the tasks or act on them.
- The facade only offers `complete()`. There is no assign, start, approve, reject, cancel, escalate,
  execute, undo, snapshot or restore.
- It publishes no events, so the Observer listeners would never run.

**What the gateway does instead:** the same steps as `WorkflowManager::createInstance/start/complete`,
with the same real classes:
- clone from `DefinitionRegistry`;
- create a per-instance `ResourceCoordinator` and `CommandProcessor`;
- add dependencies from `getDependencies()`;
- run the real commands;
- route with the real `TaskRouter`;
- publish `Event`s on the instance's `WorkflowEventBus`.

**Swap:** add accessors to `WorkflowManager` (e.g. `WorkflowInstance* getInstance(id)`,
`WorkComponent* getTask(instanceId, taskId)`, `CommandProcessor* getCommands(id)`,
`WorkflowEventBus* getEvents(id)`), or add the missing operations. Then make `EngineGateway` hold a
`WorkflowManager` and call it. `main.cpp` and `public/` stay unchanged.

### 2. `scenarios/TaskLookup`: stands in for task lookup by ID (Member 7)

`WorkflowManager` keeps its `components` / `items` maps private, and `WorkComponent` has no `getId()`.
TaskLookup walks the tree with `createIterator()` and unwraps decorators with the real
`getWrappedComponent()`, exactly as `WorkflowManager::createInstance` does internally.
**Swap:** use the Facade's lookup once it exists (see 1).

### 3. `scenarios/StageNames`: stands in for a name on `WorkStage` (Member 1)

`CodeDefinitionBuilder::addStage(name, parent)` creates an unnamed `WorkStage`. StageNames matches each
built stage to the spec stage with the same depth and the same set of tasks beneath it. This stays
correct even though `addDecorator` moves a decorated task to the end of its stage.
**Swap:** when `WorkStage` stores its name, read it in `EngineGateway::treeNode` and delete StageNames.

### 4. `EngineGateway::syncWorkload`: stands in for workload updates (Members 4 / 6)

`WorkloadAssignment` picks the participant with the lowest `Participant::workload()`, but no engine
class calls `itemAssigned()` / `itemReleased()`. So the gateway calls those real methods when a task's
holder changes (Assigned / InProgress count; anything else releases). **Swap:** when `AssignCommand`
(or `WorkItem` / the Mediator) updates workload, delete `syncWorkload`.

### 5. Case-data copy beside each snapshot: stands in for `InstanceSnapshot` saving data (Member 5)

`WorkflowInstance::createSnapshot()` saves the status and the tree, but not `getData()`.
**Swap:** when the snapshot stores the data map, delete `Instance::snapshotData`.

---

## Engine behaviour the GUI shows as-is (not worked around)

| Behaviour | Owner | What the GUI does |
|---|---|---|
| A command's `undo()` restores only its own task: unlocked dependants, cancelled branches and taken resources stay | Member 5 | Shows the real result; each rule-driven cancel is its own command, so it is undone separately |
| `addDecorator` only knows `"audit"`; its sink is hard-wired to `std::cout` | Members 3/7 | Only "audit" is offered in specs and in the editor; the console is captured and the audit lines are shown |
| Approval handlers call `approve()`/`reject()` on the raw `ApprovalTask`, bypassing its decorators | Member 4 | Shown in the trace |
| `RoleApproverHandler` / `FinalApproverHandler` declare their own `name` member, which is never set, so `getDecidedBy()` is empty. Their non-virtual `handleRequest` declarations have no definition and would not link if called on the derived type | Member 4 | Not called that way. `ApprovalTask::getChainDecidedBy()` falls back to the last handler the request visited. The trace names come from the base `ApprovalHandler` name, which is set, so no web stand-in is needed |
| `DefinitionRegistry::registerDefinition` takes a raw pointer; `getDefinition` returns a `shared_ptr` | Members 3/7 | Registers a copy (`new WorkflowDefinition(*built)`) |
| The Mediator only supports AND dependencies (`canBecomeAvailable` needs every prerequisite complete) | Member 4 | Decisions cancel the branches not taken; a task after an either/or split cannot wait for "whichever branch ran", so each branch must finish on its own (documented in the editor) |
| `TaskRouter` strategies call `WorkItem::assignTo` directly, not through `AssignCommand` | Member 6 | Auto-assignment is shown as not undoable |
| `execute()` has no command | Member 5 | Called directly; labelled "not undoable" |
| `DeadlineEscalator` is not wired to a clock | Member 6 | Not shown |

None of these crash the GUI, and none are impossible to work around.
