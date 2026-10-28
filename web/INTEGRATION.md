# Integration status: what is real, what is still a stand-in

The web demo must call the real C++ engine, never a hard-coded copy of it. After the team merged
all patterns into main, almost everything in `web/` now runs on the real engine classes. A few
small **temporary stand-ins** remain where a teammate's class does not compile, does not link, or
lacks an accessor. Each one uses **only real engine methods**. None of them reimplements state
transitions, cloning, dependency checks, slot counting or approval decisions.

Every stand-in file starts with:

```
// TEMPORARY - stands in for <class> (Member N) until <specific fix>. Delete when that lands.
```

In the UI, a step done by a stand-in is labelled **"(temporary web stand-in)"** in the
*Patterns in action* panel. A pattern that does not build is labelled **"not yet integrated"**.

Members: 1 Composite + Iterator, 2 State + Template Method, 3 Decorator + Prototype,
4 Mediator + Chain, 5 Command + Memento, 6 Observer + Strategy, 7 Builder + Facade.
The Adapter pattern was removed by team decision. Automated tasks are automated steps
(`AutomatedTask::setStepSucceeds`).

All engine access goes through **one class, `web/server/EngineGateway`**. Its public methods mirror
`WorkflowManager`. `main.cpp` and the browser never touch the engine directly.

---

## Now real (stand-in deleted)

| Pattern | Real classes used by the web demo | Stand-in removed |
|---|---|---|
| Builder | `CodeDefinitionBuilder` (`addStage`, `addTask`, `addDependency`, `setApprovalChain`, `addDecorator`), `EnrolmentDirector`, `HealthcareDirector` | `RecordingDefinitionBuilder` |
| Prototype | `DefinitionRegistry` (both definitions), `WorkflowDefinition::cloneRoot`, `getDependencies` | definition kept in the gateway |
| Composite / Iterator | `WorkStage`, `DepthFirstIterator`, `ReadyTaskIterator` ("Available work") | own available-work filter |
| Decorator | `AuditDecorator` on one task per industry, applied by `addDecorator("audit")` | - (was not integrated) |
| Command | `CommandProcessor` (+ `getHistory()`), `MakeAvailable/Assign/Start/Complete/Approve/Reject/Cancel/EscalateCommand` | `WebCommands`, `HistoryLog` |
| Memento | `WorkflowInstance::createSnapshot/restoreSnapshot`, `getData/setData`, `SnapshotHistory` | - |
| Mediator | `ResourceCoordinator` (one per instance), `Resource` with real places (`take`/`giveBack`), `Participant(id, name, role)` | `ParticipantDirectory` |
| Observer | `WorkflowEventBus` (one per instance), `DependencyActivator`, `AuditLogger`, `NotificationSender`, `MonitoringCollector` | the Observer half of `PendingIntegration` |
| Adapter | removed from the project | `TestConnectors`, `temporary_stubs/` |

---

## Remaining stand-ins

### 1. `server/EngineGateway` internals: stand in for `WorkflowManager` (Facade, Member 7)

**Why it can't be used:**
- `src/WorkflowManager.cpp` compiles, but it **cannot link**: it calls `TaskRouter::setStrategy` and
  `TaskRouter::route`, and `TaskRouter.cpp` does not compile (see Strategy below).
- Even if it linked, the GUI could not use it. Its `Runtime` (instance, mediator, command
  processor, components) is private, and it has no way to read an instance's tree or tasks. It also
  offers no assign, start, approve, reject, cancel, escalate, execute, undo or snapshot operations.
- `WorkflowManager` publishes no events, so it could not drive the Observer listeners either.

**What the gateway does instead:** the same steps as `WorkflowManager::createInstance/start/complete`,
using the same real classes:
- clone from `DefinitionRegistry`;
- create a per-instance `ResourceCoordinator` and `CommandProcessor`;
- wire dependencies from `getDependencies()`;
- run the real commands.

It also publishes `Event`s on the instance's `WorkflowEventBus`, so the real listeners react.

**Swap when the Facade can be used:**
1. Fix Strategy so `WorkflowManager.cpp` links; remove it from `$ExcludedEngineFiles` in `build.ps1`.
2. Add accessors to `WorkflowManager`, e.g. `WorkflowInstance* getInstance(id)`,
   `WorkComponent* getTask(instanceId, taskId)`, `CommandProcessor* getCommands(id)` and
   `WorkflowEventBus* getEvents(id)`. Or add the missing operations: assign, start, approve, reject,
   cancel, escalate, execute, undo, snapshot, restore.
3. Make `EngineGateway` hold a `WorkflowManager` and call it from `createInstance/start/complete/getStatus`
   and the new operations. `main.cpp` and `public/` stay unchanged.

### 2. `scenarios/TaskLookup`: stands in for task lookup by ID (Member 7)

**Why:** `WorkflowManager` keeps its `components` / `items` maps private, and `WorkComponent` has no
`getId()`. TaskLookup walks the tree with `createIterator()`, unwrapping decorators with the real
`WorkComponentDecorator::getWrappedComponent()`, exactly as `WorkflowManager::createInstance` does.
It returns the outermost node (commands act on it, so decorator hooks run) and the inner `WorkItem`
(identity, `isComplete`, events).

**Swap:** use the Facade's task lookup once it exists (see 1), then delete `TaskLookup.h/.cpp`.

### 3. `scenarios/StageNameRecorder`: stands in for a name on `WorkStage` (Member 1)

**Why:** `CodeDefinitionBuilder::addStage(name, parent)` creates an unnamed `WorkStage`. StageNameRecorder
is a `DefinitionBuilder` that forwards **every** step unchanged to the real `CodeDefinitionBuilder`.
It only notes each stage's name by tree position.

**Swap:** when `WorkStage` stores its name (e.g. `getName()`), read it in `EngineGateway::treeNode`, pass the
real `CodeDefinitionBuilder` straight to the directors, and delete `StageNameRecorder.h/.cpp`.

### 4. `scenarios/ChainTrace`: stands in for path reporting by the approval handlers (Member 4)

**Why:** `RoleApproverHandler` / `FinalApproverHandler` decide correctly, but report nothing. Their role
and limit are private, and they publish no event. `TracingRoleApproverHandler` /
`TracingFinalApproverHandler` subclass them, report "request reached me", then call the **real**
`handleRequest`, which decides.

**Swap:** when the handlers publish an event or expose `getRole()` / `getLimit()`, use the plain handlers
in `EducationScenario` / `HealthcareScenario` and delete `ChainTrace.h/.cpp`.

### 5. `server/PendingIntegration`: stands in for `TaskRouter` / `AssignmentStrategy` (Strategy, Member 6)

**Why:** none of the Strategy classes compile.
- `include/AssignmentStrategy.h` declares `virtual void route(WorkItem*) = 0;`.
- `include/RoleBasedAssignment.h` and `include/RuleBasedAssignment.h` declare `bool route(WorkItem*) override;`,
  which gives *conflicting return type*.
- `include/TaskRouter.h` and `include/WorkloadAssignment.h` declare `void route`, but
  `src/TaskRouter.cpp` and `src/WorkloadAssignment.cpp` define `bool route`, which gives *no declaration matches*.

`build.ps1` therefore skips those four files. Assignment is manual, and the Patterns panel says so.
The builder's `setAssignmentStrategy` is real but has nothing to store.

**Swap:** make `route()` agree (probably `bool` everywhere), then remove the four files from
`$ExcludedEngineFiles`. In `EngineGateway::build()`, create one strategy per task (e.g. `RoleBasedAssignment`
with the industry's `Participant`s) and call `CodeDefinitionBuilder::setAssignmentStrategy`. Then replace
`PendingIntegration::chooseParticipant` with an "auto-assign" action that calls
`TaskRouter::setStrategy(definition->getStrategy(task))` and `route(item)`, and shows the strategy's name.
Keep the manual pick as an override, then delete `PendingIntegration.h/.cpp`.

### 6. Case-data copy beside each snapshot: stands in for `InstanceSnapshot` saving instance data (Member 5)

**Why:** `WorkflowInstance::createSnapshot()` saves the status and a clone of the tree, but not
`getData()`. Restoring would keep the newer case data (urgency, decision, slot holder).

**Swap:** when `InstanceSnapshot` stores the data map and `restoreSnapshot` restores it, delete
`Instance::snapshotData` in `EngineGateway.cpp`.

### 7. Build flag (approved earlier)

`include/ApproveCommand.h` uses `WorkComponent`, `WorkItemState` and `Participant` without declaring
them, so `src/ApproveCommand.cpp` does not compile on its own. `build.ps1` compiles that one file with
`-include WorkComponent.h`, and `EngineGateway.cpp` includes `WorkComponent.h` before `ApproveCommand.h`.
Remove the flag when the header has `class WorkComponent; class WorkItemState; class Participant;`.

The earlier `-include cstddef` and `-I <project root>` flags are no longer needed and were removed.

---

## Engine behaviour the GUI shows as-is (not worked around)

| Behaviour | Owner | What the GUI does |
|---|---|---|
| A command's `undo()` restores only its own task. It does not undo branch cancellations by rules, dependants unlocked by `DependencyActivator`, or a slot taken by `requestResource` | Member 5 | Shows the real result. Each rule cancellation is its own `CancelCommand`, so it is undone separately. |
| `CodeDefinitionBuilder::addDecorator` only knows `"audit"`; `SecureDecorator`, `ValidationDecorator` and `PriorityDecorator` need arguments it cannot pass | Members 3/7 | Uses `AuditDecorator` (one task per industry); the other three are compiled but not applied |
| `addDecorator` removes the task and re-adds the decorator at the END of its stage, changing task order | Member 7 | Decorates only tasks that are already last in their stage |
| `AuditDecorator`'s sink is hard-wired to `std::cout` in `CodeDefinitionBuilder` | Member 7 | The gateway captures the console and shows each audit line in the Patterns panel |
| Approval handlers call `approve()` / `reject()` on the raw `ApprovalTask`, bypassing its decorators, so the chain's decision is not audited | Member 4 | Shown in the trace text |
| `DefinitionRegistry::registerDefinition` takes ownership of a raw pointer, but `CodeDefinitionBuilder::getDefinition` returns a `shared_ptr` | Members 3/7 | Registers a copy (`new WorkflowDefinition(*built)`; the copy shares the prototype root) |
| `WorkflowDefinition::keepApprovalHandler` is empty | Member 7 | Not used: the scenario owns its chain |
| Nothing in the engine publishes events | Members 6/7 | The gateway publishes after each transition; the real listeners do the rest |
| `DeadlineEscalator` is not wired to a clock | Member 6 | Not shown |
| `execute()` has no command | Member 5 | Called directly; labelled "not undoable" |
