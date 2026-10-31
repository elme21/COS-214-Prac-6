// TIVIDY live demo - browser side.
// This file contains NO workflow logic and NO industry: every scenario comes from the server as data
// (a spec), and every button calls the C++ server, which calls the real engine through EngineGateway.
'use strict';

const STATES = ['Created', 'Available', 'Assigned', 'InProgress', 'Completed', 'Rejected', 'Cancelled', 'Escalated'];

// Every lifecycle operation is always offered; the engine decides whether it is allowed.
const ACTIONS = [
  { key: 'makeAvailable', label: 'makeAvailable' },
  { key: 'start', label: 'start' },
  { key: 'execute', label: 'execute', title: 'Run the Template Method: checkReady -> onEnter -> perform -> recordResult' },
  { key: 'complete', label: 'complete' },
  { key: 'approve', label: 'approve' },
  { key: 'reject', label: 'reject' },
  { key: 'cancel', label: 'cancel' },
  { key: 'escalate', label: 'escalate' },
];

// Status of each pattern in this build, shown on the "Generic engine" tab.
// working = real engine classes run; partial = real classes run with a known gap;
// missing = cannot be used by the GUI.
const PATTERNS = [
  ['Facade', 'WorkflowManager', 'missing', 'Not used by the GUI. WorkflowManager keeps every instance\'s tree, mediator and command history private, so a case it creates cannot be shown or acted on, and its complete() needs an InProgress task that no Facade operation can produce. EngineGateway mirrors its interface and does the same steps with the real classes.'],
  ['Builder', 'DefinitionBuilder, CodeDefinitionBuilder (+ GenericDirector)', 'working', 'Every scenario - template or new - is built by one GenericDirector on the real CodeDefinitionBuilder: stages, nested sub-stages, tasks, dependencies, chains, strategies and decorators.'],
  ['Prototype', 'WorkComponent::clone, WorkflowDefinition::cloneRoot, DefinitionRegistry', 'working', 'Every definition sits in the real DefinitionRegistry; every instance is a deep clone of one.'],
  ['Composite', 'WorkComponent, WorkStage, WorkItem', 'working', 'Stages with nested sub-stages and tasks, walked through getChild().'],
  ['Iterator', 'WorkIterator, DepthFirstIterator, ReadyTaskIterator', 'working', '"Available work" is the real ReadyTaskIterator; task lists use DepthFirstIterator.'],
  ['Decorator', 'WorkComponentDecorator, AuditDecorator (+ Secure, Validation, Priority)', 'partial', 'Tasks marked "audit" in a spec are wrapped by CodeDefinitionBuilder::addDecorator; its audit lines appear in the Patterns panel. The builder only offers "audit".'],
  ['Template Method', 'WorkItem::execute + ManualTask / AutomatedTask / ApprovalTask::perform', 'working', 'execute runs the real skeleton. Automated steps succeed or fail per the spec (setStepSucceeds).'],
  ['State', 'WorkItemState + 8 concrete states', 'working', 'Every refusal you see comes from a state returning false.'],
  ['Command', 'WorkflowCommand, CommandProcessor + 8 concrete commands', 'working', 'Every action runs a real command; Undo pops getHistory(). A command\'s undo restores only its own task.'],
  ['Memento', 'WorkflowInstance, InstanceSnapshot, SnapshotHistory', 'working', 'Snapshot / Restore use the real classes; the snapshot saves the status, the case data and a deep clone of the tree.'],
  ['Mediator', 'WorkCoordinator, ResourceCoordinator, Colleague, Participant, Resource', 'working', 'Dependencies and live resource contention: requestResource() takes a place, release() gives it back.'],
  ['Chain of Responsibility', 'ApprovalHandler, RoleApproverHandler, FinalApproverHandler, ApprovalRequest', 'working', 'Generic levels built from the spec; the real handlers decide. The path, decider and outcome are read from the result ApprovalTask keeps (getChainPath, getChainDecidedBy, getChainOutcome).'],
  ['Observer', 'WorkflowEventBus, DependencyActivator, AuditLogger, NotificationSender, MonitoringCollector', 'working', 'Each instance has its own bus; DependencyActivator unlocks dependants; the audit trail is AuditLogger::getLog(). EngineGateway publishes the events.'],
  ['Strategy', 'TaskRouter, AssignmentStrategy, RoleBasedAssignment, WorkloadAssignment', 'working', '"auto-assign" routes a task with the strategy its spec names. Workload is fed by the gateway (no engine class updates it).'],
];

const GENERIC_CLASSES = [
  'WorkComponent', 'WorkStage', 'WorkItem', 'ManualTask', 'AutomatedTask', 'ApprovalTask',
  'WorkItemState', 'CreatedState', 'AvailableState', 'AssignedState', 'InProgressState', 'CompletedState',
  'RejectedState', 'CancelledState', 'EscalatedState', 'WorkIterator', 'DepthFirstIterator', 'ReadyTaskIterator',
  'DefinitionBuilder', 'CodeDefinitionBuilder', 'DefinitionRegistry', 'WorkflowDefinition', 'WorkflowInstance',
  'InstanceSnapshot', 'SnapshotHistory', 'WorkflowCommand', 'CommandProcessor', 'MakeAvailableCommand',
  'AssignCommand', 'StartCommand', 'CompleteCommand', 'ApproveCommand', 'RejectCommand', 'CancelCommand',
  'EscalateCommand', 'WorkComponentDecorator', 'AuditDecorator', 'WorkCoordinator', 'ResourceCoordinator',
  'Colleague', 'Participant', 'Resource', 'ApprovalHandler', 'ApprovalRequest', 'RoleApproverHandler',
  'FinalApproverHandler', 'TaskRouter', 'AssignmentStrategy', 'RoleBasedAssignment', 'WorkloadAssignment',
  'Event', 'EventPublisher', 'EventListener', 'WorkflowEventBus', 'DependencyActivator', 'AuditLogger',
  'NotificationSender', 'MonitoringCollector',
];

const TEMPORARY = [
  ['server/EngineGateway (internals)', 'WorkflowManager (Facade): it has no accessors for an instance\'s tasks', 'Member 7'],
  ['scenarios/TaskLookup', 'finding a task by ID in an instance (kept private in WorkflowManager)', 'Member 7'],
  ['scenarios/StageNames', 'a name on WorkStage (it declares one, but nothing can set or read it)', 'Member 1'],
  ['EngineGateway::syncWorkload', 'updating Participant workload (no engine class calls itemAssigned)', 'Members 4 / 6'],
];

const ui = {
  scenarios: [],         // from /api/scenarios
  templates: [],         // from /api/templates
  templateErrors: [],
  tab: null,             // scenario key, 'engine' or 'editor'
  panes: {},             // scenario key -> [instanceId|null, instanceId|null]
  instances: {},         // id -> full instance view
  list: [],              // instance summaries
  selected: {},          // instanceId -> taskId
  picks: {},             // instanceId/taskId -> participant id
  branches: {},          // instanceId/taskId -> branch id
  start: {},             // scenario key -> { seed, values }
  banners: {},
  changed: {},
  last: null,
  resources: {},
  editor: null,          // { spec, errors, message, from }
  busy: false,
};

// ---------------------------------------------------------------- helpers

function esc(value) {
  return String(value === undefined || value === null ? '' : value)
    .replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

function scenarioByKey(key) { return ui.scenarios.find((s) => s.key === key) || null; }

async function api(method, path, body) {
  try {
    const response = await fetch(path, {
      method,
      headers: body ? { 'Content-Type': 'application/json' } : {},
      body: body ? JSON.stringify(body) : undefined,
    });
    const data = await response.json().catch(() => ({ ok: false, message: 'The server sent an unreadable reply.' }));
    if (!response.ok && data.ok === undefined) data.ok = false;
    return data;
  } catch (e) {
    return { ok: false, message: 'Cannot reach the C++ server. Is run.ps1 still running?' };
  }
}

function tasksOf(node, out = []) {
  if (!node) return out;
  if (node.type === 'task') out.push(node);
  (node.children || []).forEach((c) => tasksOf(c, out));
  return out;
}

function findTaskView(instance, taskId) { return tasksOf(instance.tree).find((t) => t.id === taskId); }

function stateChip(s) { return `<span class="state st-${esc(s)}">${esc(s)}</span>`; }

function money(v) { return Number(v).toLocaleString('en-ZA'); }

// ---------------------------------------------------------------- data flow

async function loadScenarios() {
  const data = await api('GET', '/api/scenarios');
  if (!data || !data.scenarios) {
    document.getElementById('app').innerHTML = `<p class="loading">${esc(data && data.message || 'Could not load the engine.')}</p>`;
    return false;
  }
  ui.scenarios = data.scenarios;
  ui.templateErrors = data.templateErrors || [];
  ui.scenarios.forEach((s) => { if (!ui.panes[s.key]) ui.panes[s.key] = [null, null]; });
  if (!ui.tab || (ui.tab !== 'engine' && ui.tab !== 'editor' && !scenarioByKey(ui.tab))) {
    ui.tab = ui.scenarios.length ? ui.scenarios[0].key : 'engine';
  }
  return true;
}

async function loadTemplates() {
  const data = await api('GET', '/api/templates');
  ui.templates = data && data.templates ? data.templates : [];
}

async function refreshList() {
  const list = await api('GET', '/api/instances');
  ui.list = Array.isArray(list) ? list : [];
}

async function refreshResources(key) {
  const s = scenarioByKey(key);
  if (!s || !s.resources.length) return;
  const r = await api('GET', '/api/resources/' + encodeURIComponent(key));
  ui.resources[key] = Array.isArray(r) ? r : [];
}

function remember(result, instanceId, title) {
  if (result.instance) ui.instances[result.instance.id] = result.instance;
  if (instanceId) {
    ui.banners[instanceId] = { ok: !!result.ok, warning: !!result.warning, message: result.message || '' };
    ui.changed[instanceId] = new Set((result.changes || []).map((c) => c.task));
  }
  ui.last = { title, ok: !!result.ok, warning: !!result.warning, message: result.message || '',
              trace: result.trace || [], console: result.console || [], isolation: null };
}

/** Re-reads every OTHER instance of the scenario from the engine and reports whether any task changed. */
async function isolationCheck(actedOn, before) {
  const others = Object.keys(before);
  if (!others.length) return null;
  const lines = [];
  let anyChanged = false;
  for (const id of others) {
    const fresh = await api('GET', '/api/instances/' + encodeURIComponent(id));
    if (!fresh || !fresh.tree) continue;
    ui.instances[id] = fresh;
    const after = Object.fromEntries(tasksOf(fresh.tree).map((t) => [t.id, t.state]));
    const diff = Object.keys(after).filter((k) => after[k] !== before[id][k]).length;
    if (diff) anyChanged = true;
    lines.push(`${id}: ${diff} of ${Object.keys(after).length} task states changed`);
  }
  return { ok: !anyChanged, text: `Isolation check after acting on ${actedOn}: ` + lines.join('; ') };
}

function statesBefore(scenario, exceptId) {
  const before = {};
  ui.list.filter((s) => s.scenario === scenario && s.id !== exceptId).forEach((s) => {
    const view = ui.instances[s.id];
    if (view) before[s.id] = Object.fromEntries(tasksOf(view.tree).map((t) => [t.id, t.state]));
  });
  return before;
}

async function run(instanceId, title, call) {
  if (ui.busy) return;
  ui.busy = true;
  const view = ui.instances[instanceId];
  const scenario = view ? view.scenario : ui.tab;
  const before = statesBefore(scenario, instanceId);
  const result = await call();
  remember(result, instanceId, title);
  ui.last.isolation = await isolationCheck(instanceId, before);
  await refreshList();
  await refreshResources(scenario);
  ui.busy = false;
  render();
}

function taskAction(instanceId, taskId, action) {
  const key = instanceId + '/' + taskId;
  const body = {};
  if (action === 'assign') body.participant = ui.picks[key] || '';
  if (action === 'complete' && ui.branches[key]) body.branch = ui.branches[key];
  const view = ui.instances[instanceId];
  const task = view && findTaskView(view, taskId);
  const label = action === 'assign' ? 'assignTo' : action === 'autoAssign' ? 'auto-assign' : action;
  return run(instanceId, `${label}() on ${task ? task.name : taskId} (${instanceId})`, () =>
    api('POST', `/api/instances/${encodeURIComponent(instanceId)}/tasks/${encodeURIComponent(taskId)}/${action}`, body));
}

function instanceOp(instanceId, op) {
  const labels = { start: 'Start instance', undo: 'Undo last action', snapshot: 'Take snapshot', restore: 'Restore latest snapshot' };
  return run(instanceId, `${labels[op]} (${instanceId})`, () => api('POST', `/api/instances/${encodeURIComponent(instanceId)}/${op}`));
}

function startForm(scenario) {
  if (!ui.start[scenario.key]) {
    const seed = scenario.seeds.length ? 0 : -1;
    ui.start[scenario.key] = { seed, values: seed >= 0 ? { ...scenario.seeds[0].data } : {} };
  }
  return ui.start[scenario.key];
}

async function createInstance(key) {
  if (ui.busy) return;
  const scenario = scenarioByKey(key);
  const form = startForm(scenario);
  ui.busy = true;
  const result = await api('POST', '/api/instances', { scenario: key, inputs: form.values });
  if (result.instance) {
    const id = result.instance.id;
    remember(result, id, `Create instance for ${result.instance.label}`);
    const panes = ui.panes[key];
    const slot = panes[0] === null ? 0 : panes[1] === null ? 1 : 0;
    panes[slot] = id;
  } else {
    ui.last = { title: 'Create instance', ok: false,
                message: result.message + (result.errors && result.errors.length ? ' ' + result.errors.join('; ') : ''),
                trace: [], console: [], isolation: null };
  }
  await refreshList();
  ui.busy = false;
  render();
}

async function resetDemo() {
  if (!window.confirm('Reset the demo? Custom scenarios are removed, the templates are reloaded, and every instance is discarded.')) return;
  const result = await api('POST', '/api/reset');
  ui.panes = {};
  ui.instances = {};
  ui.selected = {};
  ui.banners = {};
  ui.changed = {};
  ui.start = {};
  ui.resources = {};
  ui.editor = null;
  ui.tab = null;
  ui.last = { title: 'Reset demo', ok: !!result.ok, message: result.message, trace: [], console: [], isolation: null };
  await loadScenarios();
  await loadTemplates();
  await refreshList();
  for (const s of ui.scenarios) await refreshResources(s.key);
  render();
}

async function openInstance(id, slot) {
  const view = await api('GET', '/api/instances/' + encodeURIComponent(id));
  if (!view || !view.tree) return;
  ui.instances[id] = view;
  const panes = ui.panes[view.scenario];
  if (panes[1 - slot] === id) panes[1 - slot] = null;
  panes[slot] = id;
  render();
}

// ---------------------------------------------------------------- rendering

/** A selector that finds "the same" form control again after the page is re-rendered. */
function focusSelector(el) {
  if (!el || !el.dataset || el === document.body) return null;
  const d = el.dataset;
  const attr = (name, value) => `[${name}="${CSS.escape(value)}"]`;
  if (d.ed !== undefined) return attr('data-ed', d.ed) + (el.type === 'radio' ? attr('value', el.value) : '');
  if (d.edList !== undefined) return attr('data-ed-list', d.edList) + attr('value', el.value);
  if (d.edFlag !== undefined) return attr('data-ed-flag', d.edFlag);
  if (d.edBool !== undefined) return attr('data-ed-bool', d.edBool);
  if (d.edAct !== undefined) return attr('data-ed-act', d.edAct) + attr('data-path', d.path) + (d.i !== undefined ? attr('data-i', d.i) : '');
  // Any other control with data-act (task rows, action buttons, wizard buttons): match all its data-* attributes.
  if (d.act) return Object.keys(d).map((k) => attr('data-' + k.replace(/[A-Z]/g, (c) => '-' + c.toLowerCase()), d[k])).join('');
  if (el.tagName === 'SUMMARY' && el.parentElement && el.parentElement.dataset.openKey !== undefined) {
    return attr('data-open-key', el.parentElement.dataset.openKey) + ' > summary';
  }
  if (el.id === 'wiz-heading') return '#wiz-heading';
  return null;
}

function render() {
  if (ui.editor) ui.editor.pendingRender = false;
  const selector = focusSelector(document.activeElement);
  renderTabs();
  const app = document.getElementById('app');
  if (ui.tab === 'engine') app.innerHTML = renderEngine();
  else if (ui.tab === 'editor' && ui.editor) app.innerHTML = renderEditor();
  else app.innerHTML = renderScenario(ui.tab);
  if (selector) {
    const again = app.querySelector(selector);
    if (again) again.focus({ preventScroll: true });
  }
}

/** Re-render after the browser has moved focus (Tab, click), so focus lands on the right field. */
function renderSoon() { setTimeout(render, 0); }

function renderTabs() {
  const tabs = ui.scenarios.map((s) =>
    `<button class="tab ${s.template ? '' : 'custom'}" type="button" role="tab" data-act="tab" data-tab="${esc(s.key)}" aria-selected="${ui.tab === s.key}" title="${esc(s.template ? 'Template: ' + s.templateFile : 'Created in the editor')}"><span class="ind">${esc(s.industry)}</span>${esc(s.name)}</button>`);
  if (ui.editor) tabs.push(`<button class="tab" type="button" role="tab" aria-selected="${ui.tab === 'editor'}" data-act="tab" data-tab="editor">New workflow</button>`);
  tabs.push(`<button class="tab" type="button" role="tab" data-act="tab" data-tab="engine" aria-selected="${ui.tab === 'engine'}">Generic engine</button>`);
  document.getElementById('tabs').innerHTML = tabs.join('');
}

function renderScenario(key) {
  const s = scenarioByKey(key);
  if (!s) return '<p class="loading">Loading...</p>';
  const panes = ui.panes[key].filter((id) => id && ui.instances[id]);
  return `
  <div class="industry-layout">
    <aside>
      ${renderStartCard(s)}
      ${renderInstanceList(s)}
      ${renderDecisions(s)}
      ${renderChains(s)}
      ${s.resources.length ? renderResources(s) : ''}
    </aside>
    <section>
      ${panes.length ? `<div class="panes">${ui.panes[key].map((id, slot) =>
        id && ui.instances[id] ? renderPane(ui.instances[id], slot) : '').join('')}</div>`
        : `<div class="card"><h2>No instance open</h2><p class="muted">Fill in the information for a case on the left (or pick an example case)
           and press <b>Create case</b>. Create two and use <b>Open beside</b> to compare them side by side.</p></div>`}
    </section>
    <aside class="side-right">${renderPatterns()}</aside>
  </div>`;
}

function renderInputField(s, input, value) {
  const id = `in-${s.key}-${input.key}`;
  const common = `id="${esc(id)}" data-act="start-input" data-scenario="${esc(s.key)}" data-key="${esc(input.key)}"`;
  if (input.type === 'choice') {
    return `<select ${common}><option value=""></option>${input.options.map((o) => `<option value="${esc(o)}" ${String(value) === o ? 'selected' : ''}>${esc(o)}</option>`).join('')}</select>`;
  }
  return `<input ${common} type="${input.type === 'number' ? 'number' : 'text'}" value="${esc(value === undefined ? '' : value)}">`;
}

function renderStartCard(s) {
  const form = startForm(s);
  const seed = form.seed >= 0 ? s.seeds[form.seed] : null;
  return `
  <div class="card">
    <h2>${esc(s.name)}</h2>
    <div class="small"><span class="tag tag-scenario">${esc(s.industry)}</span>${s.template ? '<span class="tag tag-engine">template</span>' : '<span class="tag tag-temporary">created in the editor</span>'}</div>
    <p class="small muted">${esc(s.description)}</p>
    <div class="stack">
      <label class="small" for="seed-${esc(s.key)}"><b>Start a new case</b>${tech('workflow instance')}<br>Information for this case</label>
      <select id="seed-${esc(s.key)}" data-act="pick-seed" data-scenario="${esc(s.key)}">
        <option value="-1" ${form.seed < 0 ? 'selected' : ''}>Type in my own</option>
        ${s.seeds.map((sd, i) => `<option value="${i}" ${form.seed === i ? 'selected' : ''}>Example case: ${esc(sd.label)}</option>`).join('')}
      </select>
      ${seed && seed.summary ? `<div class="small muted">${esc(seed.summary)}</div>` : ''}
      <div class="form-grid">
        ${s.inputs.map((input) => `<label for="in-${esc(s.key)}-${esc(input.key)}">${esc(input.label)}</label>${renderInputField(s, input, form.values[input.key])}`).join('')}
      </div>
      <button class="btn primary" type="button" data-act="create" data-scenario="${esc(s.key)}">Create case</button>
      <div class="note">Clones the "${esc(s.name)}" definition from the DefinitionRegistry.</div>
    </div>
  </div>`;
}

function renderInstanceList(s) {
  const items = ui.list.filter((i) => i.scenario === s.key);
  const open = ui.panes[s.key];
  return `
  <div class="card">
    <h2>Instances (${items.length})</h2>
    ${items.length ? items.map((i) => {
      const total = i.tasks || 1;
      const bar = STATES.map((st) => i.counts[st] ? `<span class="bg-${st}" style="width:${(100 * i.counts[st] / total).toFixed(1)}%" title="${st}: ${i.counts[st]}"></span>` : '').join('');
      return `
      <div class="inst-card ${open.includes(i.id) ? 'open' : ''}">
        <div class="row"><span class="title">${esc(i.id)}</span><span class="status status-${esc(i.status)}">${esc(i.status)}</span></div>
        <div class="small">${esc(i.label)}</div>
        <div class="bar">${bar}</div>
        ${i.outcome ? `<div class="small muted">${esc(i.outcome)}</div>` : ''}
        <div class="row">
          <button class="btn small" type="button" data-act="open" data-inst="${esc(i.id)}" data-slot="0">Open</button>
          <button class="btn small" type="button" data-act="open" data-inst="${esc(i.id)}" data-slot="1">Open beside</button>
        </div>
      </div>`;
    }).join('') : '<p class="empty small">None yet.</p>'}
  </div>`;
}

function renderChains(s) {
  return s.chains.map((c) => `
  <div class="card">
    <h3>Approval chain for ${esc(c.taskName)}</h3>
    <div class="chain">
      ${c.levels.map((l, i) => `${i ? '<span class="arrow">&rarr;</span>' : ''}
        <span class="link"><b>${esc(l.label)}</b>${esc(l.role)}<br>${l.final ? 'final, ' : ''}limit ${esc(money(l.limit))}</span>`).join('')}
    </div>
    <p class="note">Each level approves if the request value is within its limit, otherwise passes it on.
    The final approver rejects anything over its limit.</p>
  </div>`).join('');
}

function renderDecisions(s) {
  if (!s.decisions.length) return '';
  return `
  <div class="card">
    <h3>Decision points</h3>
    ${s.decisions.map((d) => `
      <div class="small" style="margin-bottom:.5rem"><b>${esc(d.label)}</b> after <i>${esc(d.after)}</i>
        <span class="tag ${d.mode === 'rule' ? 'tag-scenario' : 'tag-engine'}">${d.mode === 'rule' ? 'decided by rule' : 'chosen at runtime'}</span><br>
        Branches: ${d.branches.map((b) => esc(b.label)).join(' / ')}
        ${d.rules.length ? `<br><span class="note">Rules: ${d.rules.map(esc).join('; ')}</span>` : ''}</div>`).join('')}
    <p class="note">The branches not taken are cancelled with real CancelCommands.</p>
  </div>`;
}

function renderResources(s) {
  const list = ui.resources[s.key] || [];
  return `
  <div class="card">
    <h3>Resources (engine Resources)</h3>
    ${list.map((r) => `
      <div class="slot">
        <span><b>${esc(r.name)}</b><br><span class="note">${esc(r.available)} of ${esc(r.capacity)} places free${r.owner ? ' &middot; ' + esc(r.owner) : ''}</span></span>
        <span style="text-align:right">
          <span class="${r.isFree ? 'free' : 'full'}">${r.isFree ? 'isFree() = true' : 'FULL'}</span><br>
          <span class="note">${r.heldBy.length ? 'held by ' + r.heldBy.map((g) => esc(g.instance)).join(', ') : 'not held'}</span>
        </span>
      </div>`).join('')}
    <p class="note">Live from the engine: ResourceCoordinator::requestResource() takes a place, release() gives it back.</p>
  </div>`;
}

function renderPane(inst, slot) {
  const s = scenarioByKey(inst.scenario);
  const banner = ui.banners[inst.id];
  const facts = Object.entries(inst.data)
    .filter(([k]) => !k.startsWith('resource') && !k.startsWith('refused.') && k !== 'outcome')
    .map(([k, v]) => `<span class="fact"><b>${esc(k)}</b> ${esc(v)}</span>`).join('');
  const availableChips = inst.available.map((id) => {
    const t = findTaskView(inst, id);
    return `<button class="chip-btn" type="button" data-act="select" data-inst="${esc(inst.id)}" data-task="${esc(id)}">${esc(t ? t.name : id)}</button>`;
  }).join('');

  return `
  <article class="card pane">
    <div class="pane-head">
      <div class="row">
        <h2>${esc(inst.id)} &middot; ${esc(inst.label)}</h2>
        <span class="status status-${esc(inst.status)}">${esc(inst.status)}</span>
        <span class="spacer"></span>
        <button class="btn small ghost" type="button" data-act="close" data-scenario="${esc(inst.scenario)}" data-slot="${slot}" title="Close this pane">Close</button>
      </div>
      ${inst.outcome ? `<div class="small"><b>Outcome:</b> ${esc(inst.outcome)}</div>` : ''}
      <div class="row" style="margin-top:.5rem">
        <button class="btn small ${inst.started ? '' : 'primary'}" type="button" data-act="op" data-op="start" data-inst="${esc(inst.id)}">Start</button>
        <button class="btn small" type="button" data-act="op" data-op="undo" data-inst="${esc(inst.id)}" ${inst.canUndo ? '' : 'disabled'} title="CommandProcessor::undo()">Undo (${inst.commandDepth})</button>
        <button class="btn small" type="button" data-act="op" data-op="snapshot" data-inst="${esc(inst.id)}" title="WorkflowInstance::createSnapshot()">Snapshot</button>
        <button class="btn small" type="button" data-act="op" data-op="restore" data-inst="${esc(inst.id)}" ${inst.snapshots.length ? '' : 'disabled'} title="WorkflowInstance::restoreSnapshot()">Restore (${inst.snapshots.length})</button>
      </div>
      <div class="facts">${facts}</div>
    </div>
    <div class="pane-body">
      ${banner ? `<div class="banner ${banner.ok ? (banner.warning ? 'warn' : 'ok') : 'bad'}">${banner.ok ? '' : 'Refused. '}${esc(banner.message)}</div>` : ''}
      <div class="available"><b class="small" title="from the engine's ReadyTaskIterator">Available work:</b>
        ${availableChips || `<span class="empty small">${inst.started ? 'none right now' : 'press Start'}</span>`}</div>
      ${renderNode(inst, inst.tree, s, true)}
      <p class="note">"Available work" is what the engine's <code>ReadyTaskIterator</code> returns.</p>
      <details ${inst.audit.length > 3 ? 'open' : ''}>
        <summary>Audit trail - AuditLogger (${inst.audit.length})</summary>
        ${renderHistory(inst)}
      </details>
      ${inst.snapshots.length ? `<p class="note">Snapshots: ${inst.snapshots.map(esc).join(' | ')}</p>` : ''}
    </div>
  </article>`;
}

function renderNode(inst, node, s, isRoot) {
  if (node.type === 'stage') {
    return `<div class="stage ${isRoot ? 'root' : ''}">
      ${isRoot ? '' : `<div class="stage-name">${esc(node.name)}</div>`}
      ${node.children.map((c) => renderNode(inst, c, s, false)).join('')}
    </div>`;
  }
  const t = node;
  const selected = ui.selected[inst.id] === t.id;
  const changed = ui.changed[inst.id] && ui.changed[inst.id].has(t.id);
  const who = t.assignee ? `${esc(t.assignee.name)}` : '<span class="empty">unassigned</span>';
  const extra = [];
  if (t.requestValue !== undefined) extra.push(`request value ${esc(money(t.requestValue))}`);
  if (t.chainResult && t.chainResult.ran) {
    const r = t.chainResult;
    extra.push(`last chain run: ${esc(r.outcome)}${r.decided ? ' by ' + esc(r.decidedBy) : ''} (${r.path.map(esc).join(' → ')})`);
  }
  if (t.stepSucceeds === false) extra.push('<b>automated step will fail</b>');
  if (t.resourceHeld) extra.push(`holds ${esc(t.resourceHeld)}`);
  else if (t.resourceRefused && (t.state === 'Assigned' || t.state === 'Escalated')) extra.push(`REFUSED ${esc(t.resourceRefused)}`);
  if (t.branchOf) extra.push(`branch: ${esc(t.branchOf)}`);
  if (t.prerequisites && t.prerequisites.length) extra.push('after ' + t.prerequisites.map((p) => { const pt = findTaskView(inst, p); return esc(pt ? pt.name : p); }).join(' + '));
  const decorated = (t.decorators || []).map((d) => `<span class="tag tag-engine" title="wrapped by a real decorator">${esc(d)}</span>`).join('');
  const decision = t.decision ? '<span class="tag tag-scenario" title="decision point">decision</span>' : '';
  return `
    <div class="task ${selected ? 'selected' : ''} ${changed ? 'changed' : ''}" data-act="select" data-inst="${esc(inst.id)}" data-task="${esc(t.id)}" role="button" tabindex="0">
      ${stateChip(t.state)}
      <div><div class="name">${esc(t.name)} <span class="kind">${esc(t.kind)}</span>${decorated}${decision}</div>
        <div class="meta"><span class="mono">${esc(t.id)}</span> &middot; ${who}${extra.length ? ' &middot; ' + extra.join(' &middot; ') : ''}</div></div>
    </div>
    ${selected ? renderActions(inst, t, s) : ''}`;
}

function renderActions(inst, t, s) {
  const key = inst.id + '/' + t.id;
  const people = s.participants.slice().sort((a, b) => (b.role === t.role) - (a.role === t.role));
  if (!ui.picks[key]) ui.picks[key] = people[0] ? people[0].id : '';
  if (t.decision && !ui.branches[key]) ui.branches[key] = t.decision.suggested || t.decision.branches[0].id;
  return `
  <div class="actions">
    <div class="help">${esc(t.help || '')}${t.help ? '<br>' : ''}Responsible role: <b>${esc(t.role || '-')}</b>.
      ${t.resource ? `Needs a resource: <b>${esc(t.resource)}</b>.` : ''}</div>
    <div class="row">
      <select data-act="pick-person" data-key="${esc(key)}" aria-label="Participant">
        ${people.map((p) => `<option value="${esc(p.id)}" ${ui.picks[key] === p.id ? 'selected' : ''}>${esc(p.name)} - ${esc(p.role)}</option>`).join('')}
      </select>
      <button class="btn small primary" type="button" data-act="task" data-action="assign" data-inst="${esc(inst.id)}" data-task="${esc(t.id)}">assignTo</button>
      ${t.strategy ? `<button class="btn small" type="button" data-act="task" data-action="autoAssign" data-inst="${esc(inst.id)}" data-task="${esc(t.id)}" title="TaskRouter::route with ${esc(t.strategy)}">auto-assign (${esc(t.strategy)})</button>` : ''}
    </div>
    ${t.decision ? `<div class="row" style="margin-top:.4rem"><label class="small" for="br-${esc(key)}"><b>${esc(t.decision.label)}</b> (used by complete${t.decision.mode === 'rule' ? '; the rule picks' : ''}):</label>
      <select id="br-${esc(key)}" data-act="pick-branch" data-key="${esc(key)}">
        ${t.decision.branches.map((b) => `<option value="${esc(b.id)}" ${ui.branches[key] === b.id ? 'selected' : ''}>${esc(b.label)}${b.id === t.decision.suggested ? (t.decision.mode === 'rule' ? ' (rule result)' : ' (suggested)') : ''}</option>`).join('')}
      </select></div>
      ${t.decision.reason ? `<div class="note">${esc(t.decision.reason)}${t.decision.mode === 'rule' ? '. Choosing another branch is a manual override.' : ''}</div>` : ''}` : ''}
    <div class="action-grid">
      ${ACTIONS.map((a) => `<button class="btn small" type="button" data-act="task" data-action="${a.key}" data-inst="${esc(inst.id)}" data-task="${esc(t.id)}" ${a.title ? `title="${esc(a.title)}"` : ''}>${a.label}</button>`).join('')}
    </div>
    <div class="note" style="margin-top:.4rem">All operations are offered on purpose: the engine's current state decides, and refusals are shown.</div>
  </div>`;
}

function renderHistory(inst) {
  return `
    <ol class="audit">${inst.audit.slice().reverse().map((line) => `<li class="mono small">${esc(line)}</li>`).join('')}</ol>
    <p class="note">Written by the engine's <code>AuditLogger</code>, which listens to this instance's <code>WorkflowEventBus</code>
    (${esc(inst.eventsTotal)} events counted by <code>MonitoringCollector</code>).</p>
    <h4 class="small">Command history - <code>CommandProcessor::getHistory()</code> (${esc(inst.commandDepth)}, newest first)</h4>
    ${inst.commandHistory.length ? `<ol class="audit">${inst.commandHistory.map((c) => `<li class="mono small">${esc(c)}</li>`).join('')}</ol>` : '<p class="empty small">empty</p>'}
    <h4 class="small">Notifications - <code>NotificationSender::getSent()</code> (${esc(inst.notifications.length)})</h4>
    ${inst.notifications.length ? `<ol class="audit">${inst.notifications.slice(-5).reverse().map((n) => `<li class="small">${esc(n)}</li>`).join('')}</ol>` : '<p class="empty small">none</p>'}`;
}

function kindTag(kind) {
  const label = { engine: 'engine', temporary: 'temporary web stand-in', scenario: 'scenario rule', pending: 'not used' }[kind] || kind;
  return `<span class="tag tag-${esc(kind)}">${esc(label)}</span>`;
}

function renderPatterns() {
  const last = ui.last;
  const legend = `<div class="legend">${kindTag('engine')}${kindTag('scenario')}${kindTag('temporary')}${kindTag('pending')}</div>`;
  if (!last) {
    return `<div class="card"><h2>Patterns in action</h2>${legend}
      <p class="muted small">After every action this panel lists, in order, the classes that took part and the
      pattern each one plays. Steps done by a temporary web stand-in, or by a class the GUI cannot use, are labelled.</p></div>`;
  }
  return `
  <div class="card">
    <h2>Patterns in action</h2>
    <div class="small"><b>${esc(last.title)}</b></div>
    <div class="banner ${last.ok ? (last.warning ? 'warn' : 'ok') : 'bad'}" style="margin-top:.4rem">${esc(last.message)}</div>
    ${last.isolation ? `<div class="iso ${last.isolation.ok ? '' : 'bad'}">${esc(last.isolation.text)}</div>` : ''}
    ${legend}
    ${last.trace.length ? `<ol class="trace">${last.trace.map((s) => `
      <li class="k-${esc(s.kind)}">
        <span class="actor">${esc(s.actor)}</span>
        <span class="tag tag-${esc(s.kind)}">${esc(s.pattern)}</span>
        ${s.kind === 'temporary' ? '<span class="tag tag-temporary">(temporary web stand-in)</span>' : ''}
        ${s.kind === 'pending' ? '<span class="tag tag-pending">not used by the GUI</span>' : ''}
        ${s.detail ? `<div class="detail">${esc(s.detail)}</div>` : ''}
      </li>`).join('')}</ol>` : '<p class="empty small">No engine steps.</p>'}
    <details ${last.console.length ? 'open' : ''}>
      <summary>Engine console output (${last.console.length})</summary>
      <pre class="console">${last.console.length ? last.console.map(esc).join('\n') : '(the engine printed nothing)'}</pre>
    </details>
  </div>`;
}

function miniTree(node) {
  if (node.type === 'task') return `<li>${esc(node.name)} <span class="kind">${esc(node.kind)}</span>${(node.decorators || []).map((d) => ` <span class="kind">+${esc(d)}</span>`).join('')}</li>`;
  return `<li><b>${esc(node.name)}</b><ul>${node.children.map(miniTree).join('')}</ul></li>`;
}

function renderEngine() {
  return `
  <div class="card">
    <h2>One generic engine, any industry</h2>
    <p class="muted">Every scenario below - the templates and any scenario created in the editor - runs on the same
    compiled engine classes from <code>include/</code> and <code>src/</code>. The only thing that differs is the
    <b>scenario spec</b>: plain data (input fields, people, resources, stages, tasks, dependencies, chains, decisions,
    rules). One <code>GenericDirector</code> turns any spec into a definition with the real <code>CodeDefinitionBuilder</code>.</p>
    ${ui.templateErrors.length ? `<ul class="errors">${ui.templateErrors.map((e) => `<li>${esc(e)}</li>`).join('')}</ul>` : ''}
  </div>
  <div class="engine-grid">
    <div class="card">
      <h2>Engine classes every scenario reuses unchanged</h2>
      <ul class="classlist">${GENERIC_CLASSES.map((c) => `<li>${c}</li>`).join('')}</ul>
    </div>
    ${ui.scenarios.map((s) => `
    <div class="card">
      <h2>Scenario-specific data: ${esc(s.name)}</h2>
      <p class="small"><span class="tag tag-scenario">${esc(s.industry)}</span>
        ${s.template ? `template <code>web/templates/${esc(s.templateFile)}</code>` : 'created in the editor (memory only)'}</p>
      <p class="small">${s.inputs.length} input fields &middot; ${s.participants.length} participants &middot;
        ${s.resources.length} resources &middot; ${s.decisions.length} decisions &middot; ${s.chains.length} approval chains &middot; ${s.seeds.length} seed cases</p>
      <div class="mini-tree"><ul>${miniTree(s.definition)}</ul></div>
      <details><summary class="small">The spec (JSON)</summary><pre class="json">${esc(JSON.stringify(s.spec, null, 2))}</pre></details>
    </div>`).join('')}
  </div>
  <div class="card">
    <h2>Pattern status in this build</h2>
    <div class="table-wrap"><table class="ptable">
      <thead><tr><th>Pattern</th><th>Engine classes</th><th>Status</th><th>In the demo</th></tr></thead>
      <tbody>${PATTERNS.map(([p, c, st, n]) => `<tr><td><b>${esc(p)}</b></td><td><code>${esc(c)}</code></td>
        <td><span class="pill pill-${st}">${{ working: 'working', partial: 'partial', missing: 'not usable by the GUI' }[st]}</span></td>
        <td>${esc(n)}</td></tr>`).join('')}</tbody>
    </table></div>
  </div>
  <div class="card">
    <h2>Temporary web stand-ins</h2>
    <table class="ptable"><thead><tr><th>Where (web/)</th><th>Stands in for</th><th>Removed when</th></tr></thead>
    <tbody>${TEMPORARY.map(([f, r, m]) => `<tr><td><code>${esc(f)}</code></td><td>${esc(r)}</td><td>${esc(m)} fixes it</td></tr>`).join('')}</tbody></table>
    <p class="note">Details and swap steps: <code>web/INTEGRATION.md</code>.</p>
  </div>`;
}

// ---------------------------------------------------------------- workflow editor (step-by-step wizard)
// The wizard edits the same spec object as the single-page editor did and sends it through the same
// cleanSpec(), so the JSON is unchanged; only the presentation is split into six steps. Every field has
// an everyday label with the engine's name for it in small grey text.

const WIZ_STEPS = [
  { title: 'Basics', intro: 'Name the workflow and choose what information is collected for each case.' },
  { title: 'People', intro: 'Who takes part, and the role each person has. Steps are given to roles, not to names.' },
  { title: 'Steps', intro: 'Group the work into phases. A phase holds steps and may hold sub-phases.' },
  { title: 'Approvals', intro: 'For each step that needs approval: what is approved, and who approves it, in order.' },
  { title: 'Decisions', intro: 'Points where the workflow splits into different paths, and how the path is chosen.' },
  { title: 'Review', intro: 'Read the whole workflow back, fix any problems, then create it.' },
];
const INPUT_TYPES = [['text', 'Text'], ['number', 'Number'], ['choice', 'Pick from a list']];
const TASK_TYPES = [['manual', 'Done by a person', 'ManualTask'], ['automated', 'Happens automatically', 'AutomatedTask'],
                    ['approval', 'Needs approval', 'ApprovalTask']];
const STRATEGIES = [['none', 'Manually', 'no strategy'], ['role', 'By role', 'RoleBasedAssignment'],
                    ['workload', 'Least busy', 'WorkloadAssignment']];
const PICK_MODES = [['manual', 'A person chooses', 'chosen at runtime'], ['rule', 'Automatically, by a rule', 'rule']];
const RULE_OPS = [['>', 'is more than'], ['<', 'is less than'], ['==', 'equals'], ['!=', 'does not equal'],
                  ['>=', 'is at least'], ['<=', 'is at most'], ['contains', 'contains']];
const ORDINALS = ['First', 'Second', 'Third', 'Fourth', 'Fifth', 'Sixth', 'Seventh', 'Eighth', 'Ninth', 'Tenth'];
const EFFECTS = [['onComplete', 'When it is finished'], ['onReject', 'When it is rejected'], ['onCancel', 'When it is cancelled']];

function blankSpec() {
  return {
    name: '', industry: '', description: '', idPrefix: '', subjectField: 'name',
    inputs: [{ key: 'name', label: 'Name', type: 'text' }],
    participants: [{ id: 'p1', name: '', role: '' }],
    resources: [],
    stages: [{ id: 'Stage1', name: 'Stage 1', tasks: [{ id: 'Task1', name: 'Task 1', type: 'manual', role: '' }], stages: [] }],
    decisions: [],
    seeds: [],
  };
}

function openEditor(fromName) {
  let spec = blankSpec();
  if (fromName) {
    const t = ui.templates.find((x) => x.name === fromName);
    if (t) {
      spec = JSON.parse(JSON.stringify(t.spec));
      spec.name = t.spec.name + ' (copy)';
      spec.idPrefix = '';
    }
  }
  normaliseSpec(spec);
  const step = 1;
  ui.editor = { spec, errors: [], message: '', from: fromName || '', step, open: new Set(), checked: false,
                original: JSON.stringify(spec) };
  ui.tab = 'editor';
  render();
  const heading = document.getElementById('wiz-heading');
  if (heading) heading.focus({ preventScroll: true });
  window.scrollTo(0, 0);
  if (fromName) wizCheck();
}

/** Makes sure every list the form edits exists. */
function normaliseSpec(spec) {
  ['inputs', 'participants', 'resources', 'stages', 'decisions', 'seeds'].forEach((k) => { if (!Array.isArray(spec[k])) spec[k] = []; });
  const walk = (stages) => stages.forEach((st) => {
    if (!Array.isArray(st.tasks)) st.tasks = [];
    if (!Array.isArray(st.stages)) st.stages = [];
    st.tasks.forEach((t) => {
      ['dependsOn', 'decorators', 'chain'].forEach((k) => { if (!Array.isArray(t[k])) t[k] = []; });
      if (!t.type) t.type = 'manual';
    });
    walk(st.stages);
  });
  walk(spec.stages);
  spec.decisions.forEach((d) => {
    if (!Array.isArray(d.branches)) d.branches = [];
    if (!Array.isArray(d.rules)) d.rules = [];
    d.branches.forEach((b) => { if (!Array.isArray(b.tasks)) b.tasks = []; });
  });
  spec.seeds.forEach((s) => { if (!s.data || typeof s.data !== 'object') s.data = {}; });
}

function getAt(obj, path) { return path.split('.').reduce((o, k) => (o == null ? undefined : o[k]), obj); }

/** Sets a value at a dotted path, creating objects on the way (e.g. successRule.field). */
function setAt(obj, path, value) {
  const keys = path.split('.');
  const last = keys.pop();
  let parent = obj;
  keys.forEach((k) => { if (parent[k] == null || typeof parent[k] !== 'object') parent[k] = {}; parent = parent[k]; });
  if (value === undefined) delete parent[last];
  else parent[last] = value;
}

function allSpecTasks(spec) {
  const out = [];
  const walk = (stages) => stages.forEach((st) => { st.tasks.forEach((t) => out.push(t)); walk(st.stages); });
  walk(spec.stages);
  return out;
}

/** Every task with its dotted path in the spec and the name of its phase. */
function specTaskPaths(spec) {
  const out = [];
  const walk = (stages, base) => stages.forEach((st, i) => {
    st.tasks.forEach((t, k) => out.push({ t, path: `${base}.${i}.tasks.${k}`, phase: st.name || st.id }));
    walk(st.stages, `${base}.${i}.stages`);
  });
  walk(spec.stages, 'stages');
  return out;
}

function roleList(spec) { return [...new Set(spec.participants.map((p) => p.role).filter(Boolean))]; }

/** Mirrors defaultPrefix() in ScenarioSpec.cpp, for the live case-number example. */
function defaultPrefixOf(name) {
  let prefix = '';
  let start = true;
  for (const c of String(name || '')) {
    if (prefix.length >= 4) break;
    if (/[A-Za-z0-9]/.test(c)) { if (start) prefix += c.toUpperCase(); start = false; } else start = true;
  }
  return prefix || 'WF';
}

function prefixExample(spec) {
  const p = spec.idPrefix || defaultPrefixOf(spec.name);
  return `${p} → ${p}-1, ${p}-2` + (spec.idPrefix ? '' : ' (made from the workflow name while this is empty)');
}

function approverTitle(index, count) {
  if (index === count - 1) return 'Final approver';
  return (ORDINALS[index] || `Number ${index + 1}`) + ' approver';
}

// Small form widgets bound to a dotted path in the spec.
function tech(name) { return name ? ` <small class="tech">${esc(name)}</small>` : ''; }
function field(label, techName, control, cls) {
  return `<label class="ed-field${cls ? ' ' + cls : ''}"><span>${esc(label)}${tech(techName)}</span>${control}</label>`;
}
function edText(path, value, placeholder, extra) {
  return `<input type="text" data-ed="${esc(path)}" value="${esc(value || '')}" placeholder="${esc(placeholder || '')}" ${extra || ''}>`;
}
function edNum(path, value) { return `<input type="number" data-ed="${esc(path)}" data-num="1" value="${esc(value === undefined ? '' : value)}">`; }
function edSelect(path, value, options, blank) {
  return `<select data-ed="${esc(path)}">${blank !== undefined ? `<option value="">${esc(blank)}</option>` : ''}${options.map((o) => {
    const [v, l] = Array.isArray(o) ? o : [o, o];
    return `<option value="${esc(v)}" ${String(value) === String(v) ? 'selected' : ''}>${esc(l)}</option>`;
  }).join('')}</select>`;
}
function edRadios(path, value, options, legend, legendTech) {
  return `<fieldset class="ed-radios"><legend>${esc(legend)}${tech(legendTech)}</legend>${options.map(([v, l, t]) =>
    `<label><input type="radio" name="r-${esc(path)}" data-ed="${esc(path)}" value="${esc(v)}" ${String(value) === String(v) ? 'checked' : ''}> ${esc(l)}${tech(t)}</label>`).join('')}</fieldset>`;
}
function edChecks(path, selected, options) {
  const set = new Set(selected || []);
  return `<div class="ed-checks">${options.length ? options.map(([v, l]) => `<label><input type="checkbox" data-ed-list="${esc(path)}" value="${esc(v)}" ${set.has(v) ? 'checked' : ''}> ${esc(l)}</label>`).join('') : '<span class="empty small">none available yet</span>'}</div>`;
}
function edTools(listPath, index, length) {
  return `<span class="row" style="gap:.2rem;flex-wrap:nowrap">
    <button class="icon-btn" type="button" data-ed-act="up" data-path="${esc(listPath)}" data-i="${index}" ${index === 0 ? 'disabled' : ''} title="Move up" aria-label="Move up">&uarr;</button>
    <button class="icon-btn" type="button" data-ed-act="down" data-path="${esc(listPath)}" data-i="${index}" ${index === length - 1 ? 'disabled' : ''} title="Move down" aria-label="Move down">&darr;</button>
    <button class="icon-btn del" type="button" data-ed-act="remove" data-path="${esc(listPath)}" data-i="${index}" title="Remove" aria-label="Remove">&times;</button></span>`;
}
function edAdd(listPath, kind, label) {
  return `<button class="btn small" type="button" data-ed-act="add" data-path="${esc(listPath)}" data-kind="${esc(kind)}">${esc(label)}</button>`;
}
/** A collapsed section that stays open across re-renders; inUse adds the "(in use)" marker. */
function edDetails(key, title, techName, body, inUse) {
  const marker = inUse ? ` <span class="in-use">(in use${typeof inUse === 'string' ? ': ' + esc(inUse) : ''})</span>` : '';
  return `<details class="ed-adv" data-open-key="${esc(key)}" ${ui.editor.open.has(key) ? 'open' : ''}>
    <summary>${esc(title)}${tech(techName)}${marker}</summary><div class="ed-adv-body">${body}</div></details>`;
}
/** "If [field] [is more than] [value], take path [X]" - every part is a labelled control. */
function ruleSentence(base, rule, inputs, lead, branches) {
  const r = rule || {};
  return `<div class="rule-line">
    <label>${esc(lead)} ${edSelect(`${base}.field`, r.field || '', inputs, '(choose information)')}</label>
    <label><span class="sr-only">comparison</span>${edSelect(`${base}.op`, r.op || '==', RULE_OPS)}</label>
    <label><span class="sr-only">value</span>${edText(`${base}.value`, r.value, 'value')}</label>
    ${branches ? `<label>, take path ${edSelect(`${base}.branch`, r.branch, branches, '(choose a path)')}</label>` : ''}</div>`;
}

// ---- problems: the server's validator messages, rewritten in plain language

/** Turns one validator message into { step, text, tech } naming the step, person or field concerned. */
function friendlyError(raw, spec) {
  const cut = raw.indexOf(': ');
  const where = cut < 0 ? '' : raw.slice(0, cut);
  const what = cut < 0 ? raw : raw.slice(cut + 2);
  const q = (s) => `'${s}'`;
  const quoted = (s) => { const m = /'([^']*)'/.exec(s); return m ? m[1] : ''; };
  const taskName = (id) => { const t = allSpecTasks(spec).find((x) => x.id === id); return (t && t.name) || id; };
  const inputName = (key) => { const f = spec.inputs.find((x) => x.key === key); return (f && f.label) || key; };
  const shortCode = (thing) => `${thing} needs a short code that starts with a letter or digit and uses only letters, digits or _.`;
  const out = (step, text) => ({ step, text, tech: raw });
  let m;

  if (where === 'name') {
    if (/already used/.test(what)) return out(1, `Another workflow is already called ${q(quoted(what))}. Choose a different name.`);
    return out(1, 'Give the workflow a name.');
  }
  if (where === 'industry') return out(1, 'Enter the industry, for example Insurance.');
  if (where === 'idPrefix') return out(1, 'The case number prefix may only use letters and digits.');
  if (where === 'subjectField') return out(1, 'The information chosen as the case title no longer exists. Choose another.');
  if ((m = /^inputs\[(\d+)\]/.exec(where))) {
    const f = spec.inputs[Number(m[1])] || {};
    const n = q(f.label || f.key || `information ${Number(m[1]) + 1}`);
    if (/duplicate/.test(what)) return out(1, `Two pieces of information use the short code ${q(f.key)}.`);
    if (/key must/.test(what)) return out(1, shortCode(n));
    if (/two options/.test(what)) return out(1, `Give ${n} at least two choices to pick from.`);
    return out(1, `Choose what kind of answer ${n} is.`);
  }
  if ((m = /^seeds\[(\d+)\]/.exec(where))) {
    const s = spec.seeds[Number(m[1])] || {};
    const n = s.label ? q(s.label) : `Example case ${Number(m[1]) + 1}`;
    if (/label is required/.test(what)) return out(1, `${n} needs a name.`);
    if (/must be a number/.test(what)) return out(1, `In example case ${n}, ${q(inputName(quoted(what)))} must be a number.`);
    return out(1, `Example case ${n} fills in ${q(quoted(what))}, which is no longer collected.`);
  }
  if (where === 'participants') return out(2, 'Add at least one person.');
  if ((m = /^participants\[(\d+)\]/.exec(where))) {
    const p = spec.participants[Number(m[1])] || {};
    const n = q(p.name || p.id || `person ${Number(m[1]) + 1}`);
    if (/role is required/.test(what)) return out(2, `Give ${n} a role.`);
    if (/duplicate/.test(what)) return out(2, `Two people use the short code ${q(p.id)}.`);
    return out(2, shortCode(n));
  }
  if ((m = /^resources\[(\d+)\]/.exec(where))) {
    const r = spec.resources[Number(m[1])] || {};
    const n = q(r.name || r.id || `resource ${Number(m[1]) + 1}`);
    if (/capacity/.test(what)) return out(2, `Say how many can use ${n} at once (a whole number, 0 or more).`);
    if (/owner/.test(what)) return out(2, `${n} belongs to someone who is not in the people list.`);
    if (/duplicate/.test(what)) return out(2, `Two limited resources use the short code ${q(r.id)}.`);
    return out(2, shortCode(n));
  }
  if (where === 'stages') {
    if (/task/.test(what)) return out(3, 'Add at least one step.');
    return out(3, 'Add at least one phase.');
  }
  if ((m = /^stage '(.*)'$/.exec(where))) {
    const flat = [];
    const walk = (stages) => stages.forEach((st) => { flat.push(st); walk(st.stages); });
    walk(spec.stages);
    const st = flat.find((x) => x.id === m[1]);
    const n = q((st && st.name) || m[1]);
    if (/no tasks/.test(what)) return out(3, `Phase ${n} has no steps. Add a step to it or remove it.`);
    if (/duplicate/.test(what)) return out(3, `Two phases or steps use the short code ${q(m[1])}.`);
    return out(3, shortCode(`Phase ${n}`));
  }
  if ((m = /\.tasks\[\d+\] '(.*)'\.chain\[(\d+)\]$/.exec(where))) {
    const t = allSpecTasks(spec).find((x) => x.id === m[1]);
    const count = t ? t.chain.length : 0;
    return out(4, `The ${approverTitle(Number(m[2]), count).toLowerCase()} of ${q(taskName(m[1]))} needs a number for "Can approve up to".`);
  }
  if ((m = /^task '(.*)'$/.exec(where))) {
    const n = q(taskName(m[1]));
    const other = quoted(what);
    if (/responsible role is required/.test(what)) return out(3, `Choose who does ${n}.`);
    if (/no participant has the role/.test(what)) return out(3, `Nobody has the role ${q(other)} that ${n} needs. Add a person with that role in step 2, or choose another role.`);
    if (/cannot depend on itself/.test(what)) return out(3, `${n} can't wait for itself.`);
    if (/depends on unknown task/.test(what)) return out(3, `${n} waits for a step that no longer exists (${q(other)}).`);
    if (/duplicate id/.test(what)) return out(3, `Two phases or steps use the short code ${q(m[1])}.`);
    if (/^id must/.test(what)) return out(3, shortCode(`Step ${n}`));
    if (/assignee's resource/.test(what)) return out(3, `${n} needs "the assignee's own resource", but no limited resource belongs to anyone (step 2, Advanced).`);
    if (/unknown resource/.test(what)) return out(3, `${n} needs a limited resource that no longer exists (${q(other)}).`);
    if (/decorator/.test(what)) return out(3, `${n} has extra behaviour that this engine does not offer (${q(other)}); only the audit log is available.`);
    if (/strategy/.test(what)) return out(3, `Choose how a person is picked for ${n}.`);
    if (/only automated tasks have a success rule/.test(what)) return out(3, `Only steps that happen automatically can have a success rule; ${n} is not one.`);
    if (/success rule uses unknown input/.test(what)) return out(3, `The success rule of ${n} checks information that is no longer collected (${q(other)}).`);
    if (/success rule operator/.test(what)) return out(3, `The success rule of ${n} uses a comparison this engine does not know (${q(other)}).`);
    if (/approval chain with at least one level/.test(what)) return out(4, `Add at least one approver to ${n}.`);
    if (/request value uses unknown input/.test(what)) return out(4, `${n} takes its amount from information that is no longer collected (${q(other)}).`);
    if (/must be a number field/.test(what)) return out(4, `${n} takes its amount from ${q(inputName(other))}, which is not a Number.`);
    if (/only approval tasks have an approval chain/.test(what)) return out(4, `${n} has approvers but does not need approval.`);
    if (/refers to unknown task/.test(what)) return out(3, `What happens after ${n} refers to a step that no longer exists (${q(other)}).`);
    return out(3, `${n}: ${what}`);
  }
  if (where === 'dependencies') {
    const ids = [...new Set((/cycle (.*) \(/.exec(what) || ['', ''])[1].split(' -> ').filter(Boolean))];
    const names = ids.map((id) => q(taskName(id)));
    if (names.length === 1) return out(3, `${names[0]} can't wait for itself.`);
    return out(3, `${names.slice(0, -1).join(', ')} and ${names[names.length - 1]} wait for each other in a circle, so none of them can ever start.`);
  }
  if ((m = /^decisions\[(\d+)\]/.exec(where))) {
    const d = spec.decisions[Number(m[1])] || { branches: [] };
    const n = q(d.label || d.id || `split point ${Number(m[1]) + 1}`);
    const b = /\.branches\[(\d+)\]/.exec(where);
    const r = /\.rules\[(\d+)\]/.exec(where);
    if (b) {
      const br = d.branches[Number(b[1])] || {};
      const p = q(br.label || br.id || `path ${Number(b[1]) + 1}`);
      if (/at least one task/.test(what)) return out(5, `Path ${p} of ${n} has no steps. Tick at least one.`);
      if (/cannot contain/.test(what)) return out(5, `Path ${p} can't contain ${q(taskName(d.after))}, the step the split comes after.`);
      if (/in two branches/.test(what)) return out(5, `${q(taskName(quoted(what)))} is on two paths of ${n}. A step can only be on one path.`);
      if (/unknown task/.test(what)) return out(5, `Path ${p} of ${n} contains a step that no longer exists (${q(quoted(what))}).`);
      if (/duplicate/.test(what)) return out(5, `Two paths of ${n} use the short code ${q(br.id)}.`);
      return out(5, shortCode(`Path ${p}`));
    }
    if (r) {
      const k = Number(r[1]) + 1;
      if (/unknown input/.test(what)) return out(5, `Rule ${k} of ${n} needs a piece of information to check.`);
      if (/unknown branch/.test(what)) return out(5, `Rule ${k} of ${n} needs a path to take.`);
      return out(5, `Rule ${k} of ${n} uses a comparison this engine does not know.`);
    }
    if (/'after' must name a task/.test(what)) return out(5, `Choose the step after which ${n} happens.`);
    if (/already follows/.test(what)) return out(5, `Two split points come after ${q(taskName(quoted(what)))}. Only one is allowed per step.`);
    if (/two branches/.test(what)) return out(5, `${n} needs at least two paths.`);
    if (/default branch/.test(what)) return out(5, `The fallback path of ${n} no longer exists.`);
    if (/needs at least one rule/.test(what)) return out(5, `${n} is chosen automatically, so it needs at least one rule.`);
    if (/clashes/.test(what)) return out(5, `The short code of ${n} is already used by another split point or a piece of information.`);
    if (/mode/.test(what)) return out(5, `Choose how the path of ${n} is chosen.`);
    return out(5, shortCode(`Split point ${n}`));
  }
  return out(1, raw);
}

function wizProblems() { return (ui.editor.errors || []).map((e) => friendlyError(e, ui.editor.spec)); }

function renderProblemList(problems, withLinks) {
  return `<ul class="problems">${problems.map((p) => `<li>${esc(p.text)}
    ${withLinks ? ` <button class="btn small" type="button" data-act="wiz-go" data-key="${p.step}">Go to step ${p.step}: ${esc(WIZ_STEPS[p.step - 1].title)}</button>` : ''}
    <small class="tech">${esc(p.tech)}</small></li>`).join('')}</ul>`;
}

function stepProblems(step, problems) {
  const ed = ui.editor;
  if (!ed.checked) return '';
  const mine = problems.filter((p) => p.step === step);
  if (!mine.length) return '<p class="note ok-note">No problems found on this step.</p>';
  return `<div class="banner warn" style="margin-bottom:.2rem">${mine.length} problem${mine.length > 1 ? 's' : ''} on this step
    <span class="why">You can carry on; fix ${mine.length > 1 ? 'them' : 'it'} before creating the workflow.</span></div>${renderProblemList(mine, false)}`;
}

// ---- the six steps

function wizBasics(spec) {
  const ed = ui.editor;
  const allInputs = spec.inputs.map((i) => [i.key, i.label || i.key]);
  const starts = [['', 'Start from scratch', '']].concat(ui.templates.map((t) => [t.name, `Use a template: ${t.name}`, t.industry]));
  return `
    <fieldset class="ed-radios ed-start"><legend>How do you want to start?</legend>
      ${starts.map(([v, l, ind]) => `<button class="btn choice" type="button" data-act="ed-from" data-key="${esc(v)}" aria-pressed="${ed.from === v}">${esc(l)}${ind ? ` <small class="tech">${esc(ind)}</small>` : ''}</button>`).join('')}
    </fieldset>
    <p class="note">A template fills in every step; you can change anything afterwards.</p>
    <div class="ed-fields" style="margin-top:.6rem">
      ${field('Workflow name', 'WorkflowDefinition', edText('name', spec.name, 'e.g. Car insurance claim'))}
      ${field('Industry', '', edText('industry', spec.industry, 'e.g. Insurance'))}
    </div>
    <label class="ed-field" style="margin-top:.4rem"><span>Short description</span><textarea data-ed="description">${esc(spec.description || '')}</textarea></label>

    <h3 class="ed-h">Information collected for each case${tech('instance data')}</h3>
    <p class="note">Filled in when a case is started. Rules and approval amounts can read it.</p>
    ${spec.inputs.map((f, i) => `
      <div class="ed-task"><div class="row"><b class="small">Information ${i + 1}</b><span class="spacer"></span>${edTools('inputs', i, spec.inputs.length)}</div>
        <div class="ed-fields">
          ${field('What is it called?', 'label', edText(`inputs.${i}.label`, f.label, 'e.g. Claim value'))}
          ${field('Kind of answer', 'type', edSelect(`inputs.${i}.type`, f.type, INPUT_TYPES))}
          ${f.type === 'choice' ? field('Choices (separate with commas)', 'options', edText(`inputs.${i}.options`, [].concat(f.options || []).join(', '), 'a, b, c')) : ''}
          ${field('Short code', 'key', edText(`inputs.${i}.key`, f.key))}
        </div></div>`).join('')}
    <div style="margin-top:.5rem">${edAdd('inputs', 'input', '+ Add a piece of information')}</div>
    <div class="ed-fields" style="margin-top:.6rem">${field('Use as the case title', 'subjectField', edSelect('subjectField', spec.subjectField, allInputs, '(the first Text field)'))}</div>

    ${edDetails('basics-prefix', 'Advanced: Case number prefix', 'instance ID prefix', `
      <div class="ed-fields">${field('Case number prefix', 'idPrefix', edText('idPrefix', spec.idPrefix, 'e.g. CLM'))}</div>
      <p class="note">Example: <span id="prefix-example">${esc(prefixExample(spec))}</span></p>`, spec.idPrefix ? 'prefix ' + spec.idPrefix : '')}
    ${edDetails('basics-seeds', 'Advanced: Example cases (ready-made test data)', 'seed instances', `
      <p class="note">Offered on the "Start a new case" form so a case can be started with one click.</p>
      ${spec.seeds.map((s, i) => `
        <div class="ed-task"><div class="row"><b class="small">Example case ${i + 1}</b><span class="spacer"></span>${edTools('seeds', i, spec.seeds.length)}</div>
          <div class="ed-fields">${field('Name of the example', 'label', edText(`seeds.${i}.label`, s.label))}${field('One-line summary', 'summary', edText(`seeds.${i}.summary`, s.summary))}</div>
          <div class="ed-fields" style="margin-top:.3rem">${spec.inputs.filter((f) => f.key).map((f) => field(f.label || f.key, '', f.type === 'choice'
            ? edSelect(`seeds.${i}.data.${f.key}`, s.data[f.key] || '', [].concat(f.options || []).join(',').split(',').map((o) => o.trim()).filter(Boolean), '')
            : edText(`seeds.${i}.data.${f.key}`, s.data[f.key]))).join('')}</div>
        </div>`).join('')}
      <div style="margin-top:.5rem">${edAdd('seeds', 'seed', '+ Add an example case')}</div>`,
      spec.seeds.length ? `${spec.seeds.length} example case${spec.seeds.length > 1 ? 's' : ''}` : '')}`;
}

function wizPeople(spec) {
  return `
    <h3 class="ed-h">People and their roles${tech('Participant / role')}</h3>
    <p class="note">People with the same role can do the same steps. Type a new role, or reuse one from the list.</p>
    ${spec.participants.map((p, i) => `
      <div class="ed-task"><div class="row"><b class="small">Person ${i + 1}</b><span class="spacer"></span>${edTools('participants', i, spec.participants.length)}</div>
        <div class="ed-fields">
          ${field('Name', '', edText(`participants.${i}.name`, p.name, 'e.g. Thabo Nkosi'))}
          ${field('Role', 'role', edText(`participants.${i}.role`, p.role, 'e.g. Assessor', 'list="ed-roles"'))}
          ${field('Short code', 'Participant id', edText(`participants.${i}.id`, p.id))}
        </div></div>`).join('')}
    <div style="margin-top:.5rem">${edAdd('participants', 'participant', '+ Add a person')}</div>

    ${edDetails('people-resources', 'Advanced: Limited resources (e.g. rooms, appointment slots)', 'Resource - Mediator', `
      <p class="note">Something only a few cases can use at a time. A resource that belongs to a person can be asked for
        as "the assignee's own resource" (for example a doctor's appointment slots).</p>
      ${spec.resources.map((r, i) => `
        <div class="ed-task"><div class="row"><b class="small">Resource ${i + 1}</b><span class="spacer"></span>${edTools('resources', i, spec.resources.length)}</div>
          <div class="ed-fields">
            ${field('Name', '', edText(`resources.${i}.name`, r.name, 'e.g. Consulting room'))}
            ${field('How many at once', 'capacity', edNum(`resources.${i}.capacity`, r.capacity))}
            ${field('Belongs to', 'owner', edSelect(`resources.${i}.owner`, r.owner || '', spec.participants.map((p) => [p.id, p.name || p.id]), '(nobody)'))}
            ${field('Short code', 'Resource id', edText(`resources.${i}.id`, r.id))}
          </div></div>`).join('')}
      <div style="margin-top:.5rem">${edAdd('resources', 'resource', '+ Add a limited resource')}</div>`,
      spec.resources.length ? `${spec.resources.length} resource${spec.resources.length > 1 ? 's' : ''}` : '')}`;
}

function wizSteps(spec) {
  const taskOptions = allSpecTasks(spec).map((t) => [t.id, t.name || t.id]);
  const roles = roleList(spec);
  const allInputs = spec.inputs.map((i) => [i.key, i.label || i.key]);
  const ownedResources = spec.resources.some((r) => r.owner);
  const resourceOpts = (ownedResources ? [['@assignee', "the assignee's own resource"]] : []).concat(spec.resources.map((r) => [r.id, r.name || r.id]));

  const renderTask = (t, listPath, idx, count) => {
    const path = `${listPath}.${idx}`;
    const others = taskOptions.filter(([v]) => v !== t.id);
    const effectsUsed = EFFECTS.filter(([w]) => t[w] && Object.keys(t[w]).some((k) => [].concat(t[w][k] || []).length || (k === 'set' && Object.keys(t[w].set || {}).length)));
    const inUse = [].concat((t.decorators || []).includes('audit') ? ['audit log'] : [],
      t.strategy && t.strategy !== 'none' ? [(STRATEGIES.find(([v]) => v === t.strategy) || [0, t.strategy])[1].toLowerCase()] : [],
      t.resource ? ['resource'] : [], effectsUsed.length ? ['what happens afterwards'] : []).join(', ');
    const effect = ([when, title]) => {
      const e = t[when] || {};
      return `<div class="ed-sub"><b>${esc(title)}</b>${tech(when)}
        <div class="ed-fields">${field('Result text for the case', 'outcome', edText(`${path}.${when}.outcome`, e.outcome, 'e.g. Paid R{claimValue}'))}</div>
        <div class="ed-trio">
          <div><span class="ed-label">Cancel these steps${tech('cancel')}</span>${edChecks(`${path}.${when}.cancel`, [].concat(e.cancel || []), taskOptions)}</div>
          <div><span class="ed-label">Unlock these steps${tech('unlock')}</span>${edChecks(`${path}.${when}.unlock`, [].concat(e.unlock || []), taskOptions)}</div>
          <div><span class="ed-label">Free the resource held by${tech('release')}</span>${edChecks(`${path}.${when}.release`, [].concat(e.release || []), taskOptions)}</div>
        </div></div>`;
    };
    return `
    <div class="ed-task">
      <div class="row"><b class="small">Step: ${esc(t.name || t.id || 'new step')}</b>${tech('WorkItem')}<span class="spacer"></span>${edTools(listPath, idx, count)}</div>
      <div class="ed-fields">
        ${field('Step name', '', edText(`${path}.name`, t.name, 'e.g. Assess the claim'))}
        ${field('Who does this step', 'role', edSelect(`${path}.role`, t.role, roles, '(choose a role)'))}
        ${field('Short code', 'WorkItem id', edText(`${path}.id`, t.id))}
      </div>
      ${edRadios(`${path}.type`, t.type, TASK_TYPES, 'What kind of step is this?', 'WorkItem type')}
      <div class="ed-fields" style="margin-top:.3rem">${field('Instructions for the person doing it', 'help', edText(`${path}.help`, t.help, 'optional'))}</div>
      <div class="ed-sub"><span class="ed-label">Can only start after...${tech('dependency - Mediator')}</span>${edChecks(`${path}.dependsOn`, t.dependsOn, others)}
        <div class="note">Tick nothing to let it start as soon as the case starts. It waits for every ticked step.</div></div>
      ${t.type === 'automated' ? `<div class="ed-sub"><span class="ed-label">Automatic step settings${tech('setStepSucceeds')}</span>
        <div class="ed-checks"><label><input type="checkbox" data-ed-bool="${esc(path)}.succeeds" ${t.succeeds === false ? '' : 'checked'}> Normally succeeds</label></div>
        ${ruleSentence(`${path}.successRule`, t.successRule, allInputs, 'Or decide per case - it only succeeds if')}
        <div class="note">When a rule is chosen, the rule decides and the tick box is ignored; otherwise the step fails.</div></div>` : ''}
      ${t.type === 'approval' ? `<div class="ed-sub note">What is approved, and who approves it, is set in step 4.
        <button class="btn small" type="button" data-act="wiz-go" data-key="4">Go to step 4: Approvals</button></div>` : ''}
      ${edDetails('adv-' + path, 'Advanced', '', `
        <div class="ed-sub"><span class="ed-label">Extra behaviour on this step (e.g. audit log)${tech('Decorator')}</span>
          <div class="ed-checks"><label><input type="checkbox" data-ed-flag="${esc(path)}.decorators" value="audit" ${(t.decorators || []).includes('audit') ? 'checked' : ''}> Keep an audit log of this step${tech('AuditDecorator')}</label></div></div>
        <div class="ed-sub">${edRadios(`${path}.strategy`, t.strategy || 'none', STRATEGIES, 'How a person is picked', 'assignment Strategy')}
          <div class="note">Used by the Auto-assign button. A person can always be picked by hand.</div></div>
        <div class="ed-sub"><div class="ed-fields">${field('Needs a limited resource', 'Resource', edSelect(`${path}.resource`, t.resource || '', resourceOpts, '(no resource needed)'))}</div></div>
        ${EFFECTS.map(effect).join('')}`, inUse)}
    </div>`;
  };

  const renderStage = (st, listPath, idx, count, depth) => {
    const path = `${listPath}.${idx}`;
    const kind = depth ? 'Sub-phase' : 'Phase';
    return `
    <div class="ed-stage">
      <div class="row"><b>${kind}: ${esc(st.name || st.id || 'new')}</b>${tech('WorkStage - Composite')}<span class="spacer"></span>${edTools(listPath, idx, count)}</div>
      <div class="ed-fields">${field(`${kind} name`, '', edText(`${path}.name`, st.name, 'e.g. Assessment'))}${field('Short code', 'WorkStage id', edText(`${path}.id`, st.id))}</div>
      ${st.tasks.map((t, i) => renderTask(t, `${path}.tasks`, i, st.tasks.length)).join('')}
      ${st.stages.map((sub, i) => renderStage(sub, `${path}.stages`, i, st.stages.length, depth + 1)).join('')}
      <div class="row" style="margin-top:.5rem">${edAdd(`${path}.tasks`, 'task', '+ Add a step')}${edAdd(`${path}.stages`, 'stage', '+ Add a sub-phase')}</div>
    </div>`;
  };

  return `
    <h3 class="ed-h">Phases and steps${tech('WorkStage / WorkItem')}</h3>
    ${roles.length ? '' : '<p class="banner warn">Nobody has a role yet, so no step can be given to anyone. Add people in step 2.</p>'}
    ${spec.stages.map((st, i) => renderStage(st, 'stages', i, spec.stages.length, 0)).join('')}
    <div style="margin-top:.6rem">${edAdd('stages', 'stage', '+ Add a phase')}</div>`;
}

function wizApprovals(spec) {
  const approvals = specTaskPaths(spec).filter((x) => x.t.type === 'approval');
  const numberInputs = spec.inputs.filter((i) => i.type === 'number').map((i) => [i.key, i.label || i.key]);
  if (!approvals.length) {
    return `<p>None of the steps needs approval yet. In step 3, set a step's kind to <b>Needs approval</b>.</p>
      <button class="btn small" type="button" data-act="wiz-go" data-key="3">Go to step 3: Steps</button>`;
  }
  return approvals.map(({ t, path, phase }) => `
    <div class="ed-task">
      <div class="row"><h3>${esc(t.name || t.id)}</h3><span class="note">in phase ${esc(phase)}</span></div>
      <div class="ed-sub"><span class="ed-label">Amount or value to be approved${tech('request value')}</span>
        <div class="ed-fields">
          ${field('Take it from the case information', 'requestValue.field', edSelect(`${path}.requestValue.field`, t.requestValue && t.requestValue.field, numberInputs, '(no - use a fixed amount)'))}
          ${field('Fixed amount', 'requestValue.value', edNum(`${path}.requestValue.value`, t.requestValue ? t.requestValue.value : ''))}
        </div>
        <div class="note">Only Number information can be used. Each approver approves it if it is within their limit; otherwise it goes to the next approver.</div></div>
      <div class="ed-sub"><span class="ed-label">Approval order${tech('Chain of Responsibility - handler limit')}</span>
        ${t.chain.map((l, i) => `
          <div class="ed-level"><div class="row"><b class="small">${approverTitle(i, t.chain.length)}</b>
            ${i === t.chain.length - 1 ? '<span class="note">rejects anything above its limit</span>' : ''}<span class="spacer"></span>${edTools(`${path}.chain`, i, t.chain.length)}</div>
            <div class="ed-fields">
              ${field('Who approves (role)', 'role', edText(`${path}.chain.${i}.role`, l.role, 'e.g. Manager', 'list="ed-roles"'))}
              ${field('Can approve up to', 'handler limit', edNum(`${path}.chain.${i}.limit`, l.limit))}
              ${field('Name shown (optional)', 'handler name', edText(`${path}.chain.${i}.label`, l.label, i === t.chain.length - 1 ? 'Final approver' : 'Approver level ' + (i + 1)))}
            </div></div>`).join('')}
        <div style="margin-top:.5rem">${edAdd(`${path}.chain`, 'level', '+ Add an approver')}</div>
        <div class="note">A new approver is added at the end and becomes the final approver.</div></div>
    </div>`).join('');
}

function wizDecisions(spec) {
  const taskOptions = allSpecTasks(spec).map((t) => [t.id, t.name || t.id]);
  const allInputs = spec.inputs.map((i) => [i.key, i.label || i.key]);
  return `
    <h3 class="ed-h">Split points${tech('decision')}</h3>
    <p class="note">When the chosen step is finished, one path is taken and the steps on the other paths are cancelled.
      This engine only lets a step wait for ALL of its earlier steps, so let each path finish on its own.</p>
    ${spec.decisions.map((d, i) => {
      const branches = d.branches.map((b) => [b.id, b.label || b.id]);
      return `
      <div class="ed-task">
        <div class="row"><b class="small">Split point: ${esc(d.label || d.id || 'new')}</b><span class="spacer"></span>${edTools('decisions', i, spec.decisions.length)}</div>
        <div class="ed-fields">
          ${field('Name', 'label', edText(`decisions.${i}.label`, d.label, 'e.g. Claim size'))}
          ${field('Happens after step', 'after', edSelect(`decisions.${i}.after`, d.after, taskOptions, '(choose a step)'))}
          ${field('Short code', 'decision id', edText(`decisions.${i}.id`, d.id))}
        </div>
        ${edRadios(`decisions.${i}.mode`, d.mode || 'manual', PICK_MODES, 'How is the path chosen?', 'mode')}
        <div class="ed-sub"><span class="ed-label">Paths${tech('decision branch')}</span>
          ${d.branches.map((b, k) => `
            <div class="ed-level"><div class="row"><b class="small">Path ${k + 1}: ${esc(b.label || b.id)}</b><span class="spacer"></span>${edTools(`decisions.${i}.branches`, k, d.branches.length)}</div>
              <div class="ed-fields">${field('Path name', 'label', edText(`decisions.${i}.branches.${k}.label`, b.label, 'e.g. Full review'))}${field('Short code', 'branch id', edText(`decisions.${i}.branches.${k}.id`, b.id))}</div>
              <span class="ed-label">Steps on this path</span>${edChecks(`decisions.${i}.branches.${k}.tasks`, b.tasks, taskOptions.filter(([v]) => v !== d.after))}</div>`).join('')}
          <div style="margin-top:.4rem">${edAdd(`decisions.${i}.branches`, 'branch', '+ Add a path')}</div></div>
        <div class="ed-sub"><span class="ed-label">${d.mode === 'rule' ? 'Rules (the first rule that matches decides)' : 'Suggestion rules (shown to the person, who still chooses)'}${tech('rule')}</span>
          ${d.rules.map((r, k) => `<div class="row rule-row">${ruleSentence(`decisions.${i}.rules.${k}`, r, allInputs, 'If', branches)}<span class="spacer"></span>${edTools(`decisions.${i}.rules`, k, d.rules.length)}</div>`).join('')}
          <div style="margin-top:.4rem">${edAdd(`decisions.${i}.rules`, 'rule', '+ Add a rule')}</div>
          <div class="ed-fields" style="margin-top:.4rem">${field('If no rule matches, take path', 'default branch', edSelect(`decisions.${i}.default`, d.default || '', branches, '(the first path)'))}</div></div>
      </div>`;
    }).join('')}
    <div style="margin-top:.6rem">${edAdd('decisions', 'decision', '+ Add a split point')}</div>`;
}

/** The whole workflow read back as sentences. */
function wizSummary(spec) {
  const tasks = allSpecTasks(spec);
  const tn = (id) => { const t = tasks.find((x) => x.id === id); return `'${(t && t.name) || id}'`; };
  const inName = (key) => { const f = spec.inputs.find((x) => x.key === key); return (f && f.label) || key; };
  const opText = (op) => (RULE_OPS.find(([v]) => v === (op || '==')) || [0, op])[1];
  const list = (items, joiner) => (items.length < 2 ? items.join('') : items.slice(0, -1).join(', ') + ` ${joiner || 'and'} ` + items[items.length - 1]);
  const typeText = { manual: 'done by', automated: 'happens automatically', approval: 'needs approval' };
  const taskLine = (t) => {
    let line = `<b>${esc(t.name || t.id)}</b> - `;
    if (t.type === 'automated') {
      line += 'happens automatically';
      line += t.successRule && t.successRule.field
        ? `, and only succeeds if ${esc(inName(t.successRule.field))} ${esc(opText(t.successRule.op))} ${esc(t.successRule.value)}`
        : (t.succeeds === false ? ', and normally fails' : ', and normally succeeds');
      line += ` (run by the ${esc(t.role || '(no role chosen)')})`;
    } else if (t.type === 'approval') line += `needs approval (handled by the ${esc(t.role || '(no role chosen)')})`;
    else line += `${typeText.manual} the ${esc(t.role || '(no role chosen)')}`;
    line += t.dependsOn.length ? `; starts after ${esc(list(t.dependsOn.map(tn)))}` : '; can start as soon as the case starts';
    return `<li>${line}.</li>`;
  };
  const stageList = (stages) => `<ul class="summary-list">${stages.map((st) => `<li><b>${esc(st.name || st.id)}</b>
    <ul>${st.tasks.map(taskLine).join('')}</ul>${st.stages.length ? stageList(st.stages) : ''}</li>`).join('')}</ul>`;
  const approvals = tasks.filter((t) => t.type === 'approval').map((t) => {
    const rv = t.requestValue || {};
    const what = rv.field ? `the case's ${esc(inName(rv.field))}` : (rv.value !== undefined && rv.value !== '' ? `a fixed amount of ${esc(money(rv.value))}` : 'an amount of 0');
    const levels = t.chain.map((l, i) => `the ${approverTitle(i, t.chain.length).toLowerCase()} (${esc(l.role || 'no role')}) can approve up to ${esc(money(l.limit || 0))}`);
    return `<li><b>${esc(t.name || t.id)}</b> approves ${what}: ${levels.length ? esc(levels.join('; then ')) + '; anything above the final limit is rejected' : '<i>no approvers yet</i>'}.</li>`;
  });
  const splits = spec.decisions.map((d) => {
    const label = (id) => { const b = d.branches.find((x) => x.id === id); return `'${(b && (b.label || b.id)) || id}'`; };
    const paths = d.branches.map((b) => `${label(b.id)} (${b.tasks.length ? list(b.tasks.map(tn)) : 'no steps'})`);
    const rules = d.rules.map((r) => `if ${inName(r.field || '?')} ${opText(r.op)} ${r.value}, take path ${label(r.branch)}`);
    const fallback = d.default ? label(d.default) : (d.branches[0] ? label(d.branches[0].id) : '-');
    return `<li><b>${esc(d.label || d.id)}</b>: after ${esc(tn(d.after))}, the workflow splits into ${esc(list(paths, 'or'))}.
      ${d.mode === 'rule' ? 'The path is chosen automatically' : 'A person chooses the path'}${rules.length ? (d.mode === 'rule' ? ': ' : '; the suggestion is: ') + esc(rules.join('; ')) + `; otherwise ${esc(fallback)}` : ''}.</li>`;
  });
  const prefix = spec.idPrefix || defaultPrefixOf(spec.name);
  return `
    <h3>${esc(spec.name || '(no name yet)')} <span class="tag tag-scenario">${esc(spec.industry || 'no industry')}</span></h3>
    ${spec.description ? `<p class="small">${esc(spec.description)}</p>` : ''}
    <p class="small"><b>Each case collects:</b> ${spec.inputs.length ? esc(list(spec.inputs.map((f) => `${f.label || f.key} (${(INPUT_TYPES.find(([v]) => v === f.type) || [0, f.type])[1].toLowerCase()})`))) : 'nothing'}.
      Cases are numbered ${esc(prefix)}-1, ${esc(prefix)}-2, ...</p>
    <p class="small"><b>People:</b> ${spec.participants.length ? esc(list(spec.participants.map((p) => `${p.name || p.id} (${p.role || 'no role'})`))) : 'nobody yet'}.
      ${spec.resources.length ? `<br><b>Limited resources:</b> ${esc(list(spec.resources.map((r) => `${r.name || r.id} (${r.capacity} at once${r.owner ? ', belongs to ' + ((spec.participants.find((p) => p.id === r.owner) || {}).name || r.owner) : ''})`)))}.` : ''}</p>
    <p class="small"><b>Phases and steps:</b></p>${stageList(spec.stages)}
    ${approvals.length ? `<p class="small"><b>Approvals:</b></p><ul class="summary-list">${approvals.join('')}</ul>` : ''}
    ${splits.length ? `<p class="small"><b>Split points:</b></p><ul class="summary-list">${splits.join('')}</ul>` : ''}
    ${spec.seeds.length ? `<p class="small"><b>Example cases:</b> ${esc(list(spec.seeds.map((s) => s.label || '(unnamed)')))}.</p>` : ''}`;
}

function wizReview(spec, problems) {
  const ed = ui.editor;
  return `
    <div class="card review-summary">${wizSummary(spec)}</div>
    <h3 class="ed-h">Problems</h3>
    ${!ed.checked ? '<p class="note">Checking...</p>'
      : problems.length ? `<div class="banner bad" style="margin-bottom:.2rem">${problems.length} problem${problems.length > 1 ? 's' : ''} to fix before the workflow can be created</div>${renderProblemList(problems, true)}`
      : '<div class="banner ok">No problems found. The workflow is ready to create.</div>'}
    ${ed.message ? `<p class="note">Server: ${esc(ed.message)}</p>` : ''}
    <div class="row" style="margin-top:.6rem">
      <button class="btn" type="button" data-act="ed-validate">Check for problems</button>
      <button class="btn primary" type="button" data-act="ed-create">Create workflow</button>
    </div>
    ${edDetails('review-json', 'Advanced: View as data (JSON)', 'read only - exactly what is sent to the server',
      `<pre class="json">${esc(JSON.stringify(cleanSpec(spec), null, 2))}</pre>`, '')}`;
}

function renderEditor() {
  const ed = ui.editor;
  const spec = ed.spec;
  const problems = wizProblems();
  const step = ed.step;
  const bodies = [wizBasics, wizPeople, wizSteps, wizApprovals, wizDecisions];
  const progress = `
    <nav class="wiz-progress" aria-label="Workflow editor steps"><ol>
      ${WIZ_STEPS.map((s, i) => {
        const n = i + 1;
        const count = ed.checked ? (n === 6 ? problems.length : problems.filter((p) => p.step === n).length) : 0;
        return `<li class="${n === step ? 'current' : ''} ${n < step ? 'done' : ''}">
          <button class="wiz-step" type="button" data-act="wiz-go" data-key="${n}" ${n === step ? 'aria-current="step"' : ''}>
            <span class="num" aria-hidden="true">${n}</span><span class="label">${esc(s.title)}</span>
            ${count ? `<span class="count">${count}<span class="sr-only"> problem${count > 1 ? 's' : ''}</span></span>` : ''}</button></li>`;
      }).join('')}</ol>
      <div class="wiz-bar" aria-hidden="true"><span style="width:${((step - 1) / (WIZ_STEPS.length - 1) * 100).toFixed(0)}%"></span></div>
    </nav>`;
  return `
  <div class="editor">
    <datalist id="ed-roles">${roleList(spec).map((r) => `<option value="${esc(r)}"></option>`).join('')}</datalist>
    <div class="card sticky-bar wiz-head">
      <div class="row"><h2>New workflow</h2><span class="note">${ed.from ? 'based on the template ' + esc(ed.from) : 'from scratch'}</span>
        <span class="spacer"></span><button class="btn ghost" type="button" data-act="ed-cancel">Cancel</button></div>
      ${progress}
    </div>
    <div class="card ed-section">
      <h2 id="wiz-heading" tabindex="-1">Step ${step} of ${WIZ_STEPS.length}: ${esc(WIZ_STEPS[step - 1].title)}</h2>
      <p class="note">${esc(WIZ_STEPS[step - 1].intro)}</p>
      ${step < 6 ? stepProblems(step, problems) + bodies[step - 1](spec) : wizReview(spec, problems)}
    </div>
    <div class="card wiz-nav">
      <button class="btn" type="button" data-act="wiz-back" ${step === 1 ? 'disabled' : ''}>&larr; Back</button>
      <span class="spacer"></span><span class="note">Step ${step} of ${WIZ_STEPS.length}</span><span class="spacer"></span>
      ${step < 6 ? `<button class="btn primary" type="button" data-act="wiz-next">Next: ${esc(WIZ_STEPS[step].title)} &rarr;</button>`
                 : '<button class="btn primary" type="button" data-act="ed-create">Create workflow</button>'}
    </div>
  </div>`;
}

/** Moves to a step (never blocked), focuses its heading and re-checks the spec for problems. */
function wizGo(step) {
  const ed = ui.editor;
  ed.step = Math.max(1, Math.min(WIZ_STEPS.length, step));
  render();
  const heading = document.getElementById('wiz-heading');
  if (heading) heading.focus({ preventScroll: true });
  window.scrollTo(0, 0);
  wizCheck();
}

let wizCheckTimer = null;
/** Asks the server's validator (the same one Create uses) for problems; shows them per step. */
async function wizCheck(showMessage) {
  const ed = ui.editor;
  if (!ed) return;
  const result = await api('POST', '/api/scenarios/validate', cleanSpec(ed.spec));
  if (ui.editor !== ed) return;
  ed.errors = result.errors || [];
  ed.checked = true;
  if (showMessage) ed.message = result.message || '';
  // Do not rebuild the form under someone who is typing; the next render (on change or blur) shows it.
  const a = document.activeElement;
  if (a && (a.tagName === 'TEXTAREA' || (a.tagName === 'INPUT' && (a.type === 'text' || a.type === 'number')))) ed.pendingRender = true;
  else render();
}
function wizCheckSoon() {
  if (!ui.editor || !ui.editor.checked) return;
  clearTimeout(wizCheckTimer);
  wizCheckTimer = setTimeout(() => wizCheck(false), 150);
}

/** The spec as sent: comma lists split, empty optional parts dropped. */
function cleanSpec(spec) {
  const s = JSON.parse(JSON.stringify(spec));
  const csv = (v) => (Array.isArray(v) ? v : String(v || '').split(',')).map((x) => String(x).trim()).filter(Boolean);
  s.inputs.forEach((f) => { if (f.type === 'choice') f.options = csv(f.options); else delete f.options; });
  allSpecTasks(s).forEach((t) => {
    if (t.successRule && !t.successRule.field) delete t.successRule;
    if (t.requestValue) {
      if (!t.requestValue.field) delete t.requestValue.field;
      if (t.requestValue.value === '' || t.requestValue.value === undefined) delete t.requestValue.value;
      if (!Object.keys(t.requestValue).length) delete t.requestValue;
    }
    ['onComplete', 'onReject', 'onCancel'].forEach((w) => {
      const e = t[w];
      if (!e) return;
      ['cancel', 'unlock', 'release'].forEach((k) => { e[k] = csv(e[k]); if (!e[k].length) delete e[k]; });
      if (!e.outcome) delete e.outcome;
      if (!Object.keys(e).length) delete t[w];
    });
    if (t.type !== 'approval') delete t.chain;
    if (t.type !== 'automated') { delete t.succeeds; delete t.successRule; }
    if (!t.resource) delete t.resource;
    if (!t.help) delete t.help;
  });
  s.decisions.forEach((d) => { if (!d.default) delete d.default; });
  s.seeds.forEach((sd) => { Object.keys(sd.data).forEach((k) => { if (sd.data[k] === '') delete sd.data[k]; }); });
  if (!s.idPrefix) delete s.idPrefix;
  return s;
}

function newItem(kind, spec) {
  const nextId = (prefix, taken) => { let i = taken.length + 1; while (taken.includes(prefix + i)) i++; return prefix + i; };
  const stagesFlat = [];
  const walk = (stages) => stages.forEach((st) => { stagesFlat.push(st); walk(st.stages); });
  walk(spec.stages);
  const ids = allSpecTasks(spec).map((t) => t.id).concat(stagesFlat.map((st) => st.id));
  const firstRole = roleList(spec)[0] || '';
  switch (kind) {
    case 'input': return { key: nextId('field', spec.inputs.map((f) => f.key)), label: '', type: 'text' };
    case 'participant': return { id: nextId('p', spec.participants.map((p) => p.id)), name: '', role: '' };
    case 'resource': return { id: nextId('res', spec.resources.map((r) => r.id)), name: '', capacity: 1, owner: '' };
    case 'stage': return { id: nextId('Stage', ids), name: '', tasks: [], stages: [] };
    case 'task': return { id: nextId('Task', ids), name: '', type: 'manual', role: firstRole, dependsOn: [], decorators: [], chain: [] };
    case 'level': return { label: '', role: '', limit: 0 };
    case 'decision': return { id: nextId('decision', spec.decisions.map((d) => d.id)), label: '', after: '', mode: 'manual',
                              branches: [{ id: 'a', label: 'A', tasks: [] }, { id: 'b', label: 'B', tasks: [] }], rules: [] };
    case 'branch': return { id: 'branch' + Date.now() % 1000, label: '', tasks: [] };
    case 'rule': return { field: '', op: '==', value: '', branch: '' };
    case 'seed': return { label: '', summary: '', data: {} };
    default: return {};
  }
}

function editorAction(act, path, index, kind) {
  const spec = ui.editor.spec;
  const list = getAt(spec, path);
  if (!Array.isArray(list)) return;
  if (act === 'add') list.push(newItem(kind, spec));
  else if (act === 'remove') list.splice(index, 1);
  else if (act === 'up' && index > 0) [list[index - 1], list[index]] = [list[index], list[index - 1]];
  else if (act === 'down' && index < list.length - 1) [list[index + 1], list[index]] = [list[index], list[index + 1]];
  render();
  wizCheckSoon();
}

async function editorSubmit(create) {
  const spec = cleanSpec(ui.editor.spec);
  const result = await api('POST', create ? '/api/scenarios' : '/api/scenarios/validate', spec);
  ui.editor.errors = result.errors || [];
  ui.editor.message = result.message || '';
  ui.editor.checked = true;
  if (create && result.ok && result.scenario) {
    await loadScenarios();
    ui.tab = result.scenario.key;
    ui.editor = null;
    ui.last = { title: 'Create workflow', ok: true, message: result.message, trace: [], console: result.console || [], isolation: null };
  } else if (create) {
    ui.editor.step = WIZ_STEPS.length;   // problems are listed on Review
  }
  render();
  window.scrollTo(0, 0);
}

// ---------------------------------------------------------------- events

document.addEventListener('click', (event) => {
  const edBtn = event.target.closest('[data-ed-act]');
  if (edBtn) { editorAction(edBtn.dataset.edAct, edBtn.dataset.path, Number(edBtn.dataset.i), edBtn.dataset.kind); return; }
  const el = event.target.closest('[data-act]');
  if (!el) return;
  const act = el.dataset.act;
  if (act === 'tab') { ui.tab = el.dataset.tab; render(); }
  else if (act === 'create') createInstance(el.dataset.scenario);
  else if (act === 'open') openInstance(el.dataset.inst, Number(el.dataset.slot));
  else if (act === 'close') { ui.panes[el.dataset.scenario][Number(el.dataset.slot)] = null; render(); }
  else if (act === 'select') {
    if (event.target.closest('.actions')) return;
    ui.selected[el.dataset.inst] = ui.selected[el.dataset.inst] === el.dataset.task ? null : el.dataset.task;
    render();
  }
  else if (act === 'task') taskAction(el.dataset.inst, el.dataset.task, el.dataset.action);
  else if (act === 'op') instanceOp(el.dataset.inst, el.dataset.op);
  else if (act === 'ed-validate') wizCheck(true);
  else if (act === 'ed-create') editorSubmit(true);
  else if (act === 'ed-cancel') { ui.editor = null; ui.tab = ui.scenarios.length ? ui.scenarios[0].key : 'engine'; render(); }
  else if (act === 'ed-from') {
    const from = el.dataset.key;
    const edited = JSON.stringify(ui.editor.spec) !== ui.editor.original;
    if (!edited || window.confirm('Replace what you have entered with ' + (from ? 'the template "' + from + '"' : 'an empty workflow') + '?')) openEditor(from);
  }
  else if (act === 'wiz-go') wizGo(Number(el.dataset.key));
  else if (act === 'wiz-next') wizGo(ui.editor.step + 1);
  else if (act === 'wiz-back') wizGo(ui.editor.step - 1);
});

// Keep the wizard's collapsed sections open or closed across re-renders.
document.addEventListener('toggle', (event) => {
  const d = event.target;
  if (!ui.editor || !d.dataset || d.dataset.openKey === undefined) return;
  if (d.open) ui.editor.open.add(d.dataset.openKey);
  else ui.editor.open.delete(d.dataset.openKey);
}, true);

// A problem check that arrived while someone was typing is shown once they leave the field.
document.addEventListener('focusout', () => { if (ui.editor && ui.editor.pendingRender) renderSoon(); });

// Live example of the case number while the prefix (or the name it is made from) is typed.
document.addEventListener('input', (event) => {
  const d = event.target.dataset;
  if (!ui.editor || !d || (d.ed !== 'idPrefix' && d.ed !== 'name')) return;
  const example = document.getElementById('prefix-example');
  if (!example) return;
  const probe = { name: ui.editor.spec.name, idPrefix: ui.editor.spec.idPrefix };
  probe[d.ed] = event.target.value;
  example.textContent = prefixExample(probe);
});

document.addEventListener('keydown', (event) => {
  const el = event.target.closest && event.target.closest('[data-act="select"]');
  if (el && (event.key === 'Enter' || event.key === ' ')) { event.preventDefault(); el.click(); }
});

document.addEventListener('change', (event) => {
  const el = event.target;
  if (!el.dataset) return;
  const d = el.dataset;
  if (d.ed !== undefined) {
    let value = el.value;
    if (d.num) value = value === '' ? undefined : Number(value);
    setAt(ui.editor.spec, d.ed, value);
    renderSoon();
    wizCheckSoon();
  } else if (d.edList !== undefined) {
    const list = getAt(ui.editor.spec, d.edList) || [];
    setAt(ui.editor.spec, d.edList, el.checked ? list.concat([el.value]) : list.filter((v) => v !== el.value));
    renderSoon();
    wizCheckSoon();
  } else if (d.edFlag !== undefined) {
    const list = getAt(ui.editor.spec, d.edFlag) || [];
    setAt(ui.editor.spec, d.edFlag, el.checked ? [...new Set(list.concat([el.value]))] : list.filter((v) => v !== el.value));
    renderSoon();
    wizCheckSoon();
  } else if (d.edBool !== undefined) {
    setAt(ui.editor.spec, d.edBool, el.checked);
    renderSoon();
    wizCheckSoon();
  } else if (d.act === 'pick-seed') {
    const s = scenarioByKey(d.scenario);
    const i = Number(el.value);
    ui.start[s.key] = { seed: i, values: i >= 0 ? { ...s.seeds[i].data } : { ...ui.start[s.key].values } };
    render();
  } else if (d.act === 'start-input') {
    const form = ui.start[d.scenario];
    form.values[d.key] = el.value;
  } else if (d.act === 'pick-person') ui.picks[d.key] = el.value;
  else if (d.act === 'pick-branch') ui.branches[d.key] = el.value;
});

document.getElementById('resetDemo').addEventListener('click', resetDemo);
document.getElementById('newScenario').addEventListener('click', () => openEditor(''));
document.getElementById('themeToggle').addEventListener('click', () => {
  const root = document.documentElement;
  const dark = root.dataset.theme ? root.dataset.theme === 'dark' : window.matchMedia('(prefers-color-scheme: dark)').matches;
  root.dataset.theme = dark ? 'light' : 'dark';
});

(async function init() {
  if (!(await loadScenarios())) return;
  await loadTemplates();
  await refreshList();
  for (const s of ui.scenarios) await refreshResources(s.key);
  render();
})();
