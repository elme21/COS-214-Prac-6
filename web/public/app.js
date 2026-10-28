// TIVIDY live demo - browser side.
// This file contains NO workflow logic: every button calls the C++ server, which calls the
// real engine through EngineGateway. The browser only renders what the engine reports.
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
// temporary = a web stand-in does part of the job; missing = not integrated (does not build or link).
const PATTERNS = [
  ['Facade', 'WorkflowManager', 'missing', 'Not used: it cannot link without TaskRouter (Strategy does not compile) and has no way to reach an instance\'s tasks. EngineGateway mirrors its interface and does the same steps with the real classes.'],
  ['Builder', 'DefinitionBuilder, CodeDefinitionBuilder, EnrolmentDirector, HealthcareDirector', 'working', 'Both definitions are built by the real CodeDefinitionBuilder: stages, tasks, addDependency, setApprovalChain and addDecorator("audit"). StageNameRecorder (temporary) only notes stage names.'],
  ['Prototype', 'WorkComponent::clone, WorkflowDefinition::cloneRoot, DefinitionRegistry', 'working', 'Both definitions sit in the real DefinitionRegistry; every instance is a deep clone of one of them.'],
  ['Composite', 'WorkComponent, WorkStage, WorkItem', 'working', 'The tree you see is walked through getChild() on real stages and tasks.'],
  ['Iterator', 'WorkIterator, DepthFirstIterator, ReadyTaskIterator', 'working', '"Available work" comes from the real ReadyTaskIterator; task lists use DepthFirstIterator.'],
  ['Decorator', 'WorkComponentDecorator, AuditDecorator (+ Secure, Validation, Priority)', 'partial', 'One task per industry is wrapped in a real AuditDecorator through the builder; its audit lines appear in the Patterns panel. The builder only offers "audit", so the other three are compiled but not applied.'],
  ['Template Method', 'WorkItem::execute + ManualTask / AutomatedTask / ApprovalTask::perform', 'working', 'The execute button runs the real skeleton. Automated steps can fail (setStepSucceeds(false)).'],
  ['State', 'WorkItemState + 8 concrete states', 'working', 'Every refusal you see comes from a state returning false.'],
  ['Command', 'WorkflowCommand, CommandProcessor + 8 concrete commands', 'working', 'Every action runs a real command through the instance\'s CommandProcessor; Undo pops getHistory(). A command\'s undo restores only its own task.'],
  ['Memento', 'WorkflowInstance, InstanceSnapshot, SnapshotHistory', 'partial', 'Snapshot / Restore use the real classes. InstanceSnapshot does not save the instance data, so the gateway keeps a copy (temporary).'],
  ['Mediator', 'WorkCoordinator, ResourceCoordinator, Colleague, Participant, Resource', 'working', 'Dependencies and live doctor-slot contention: requestResource() takes a place, release() gives it back.'],
  ['Chain of Responsibility', 'ApprovalHandler, RoleApproverHandler, FinalApproverHandler, ApprovalRequest', 'working', 'Real handlers decide. Thin tracing subclasses (temporary) only report which handler a request reached.'],
  ['Observer', 'WorkflowEventBus, DependencyActivator, AuditLogger, NotificationSender, MonitoringCollector', 'working', 'Each instance has its own bus. DependencyActivator unlocks dependants; the audit trail is AuditLogger::getLog(). EngineGateway publishes the events, because WorkflowManager would but cannot be used.'],
  ['Strategy', 'TaskRouter, AssignmentStrategy, RoleBased / Workload / RuleBasedAssignment', 'missing', 'Does not compile (route() is void in the headers, bool in the .cpp files). Assignment is manual.'],
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
  'FinalApproverHandler', 'Event', 'EventPublisher', 'EventListener', 'WorkflowEventBus', 'DependencyActivator',
  'AuditLogger', 'NotificationSender', 'MonitoringCollector',
];

const TEMPORARY = [
  ['server/EngineGateway (internals)', 'WorkflowManager (Facade)', 'Member 7 + Member 6 (link, accessors)'],
  ['scenarios/TaskLookup', 'finding a task by ID in an instance (WorkflowManager keeps it private)', 'Member 7'],
  ['scenarios/StageNameRecorder', 'a name on WorkStage', 'Member 1'],
  ['scenarios/ChainTrace', 'approval handlers reporting which handler a request reached', 'Member 4'],
  ['server/PendingIntegration', 'TaskRouter / AssignmentStrategy (manual pick instead)', 'Member 6'],
  ['case-data copy beside each snapshot (EngineGateway)', 'InstanceSnapshot saving WorkflowInstance data', 'Member 5'],
];

const ui = {
  meta: null,
  tab: 'education',
  panes: { education: [null, null], healthcare: [null, null] },
  instances: {},         // id -> full instance view from the engine
  list: [],              // summaries
  selected: {},          // instanceId -> taskId
  picks: {},             // instanceId/taskId -> participant id
  outcomes: {},          // instanceId/taskId -> outcome
  subject: {},           // industry -> subject id
  banners: {},           // instanceId -> {ok, message}
  changed: {},           // instanceId -> Set(taskId)
  last: null,            // {title, trace, console, isolation}
  resources: {},
  busy: false,
};

// ---------------------------------------------------------------- helpers

function esc(value) {
  return String(value === undefined || value === null ? '' : value)
    .replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

function industryMeta(key) {
  return ui.meta ? ui.meta.industries.find((i) => i.key === key) : null;
}

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

function findTaskView(instance, taskId) {
  return tasksOf(instance.tree).find((t) => t.id === taskId);
}

function stateChip(s) {
  return `<span class="state st-${esc(s)}">${esc(s)}</span>`;
}

function kindLabel(kind) {
  return { manual: 'manual', automated: 'automated', approval: 'approval' }[kind] || kind;
}

// ---------------------------------------------------------------- data flow

async function loadMeta() {
  ui.meta = await api('GET', '/api/meta');
  if (!ui.meta || !ui.meta.industries) {
    document.getElementById('app').innerHTML = `<p class="loading">${esc(ui.meta && ui.meta.message || 'Could not load the engine.')}</p>`;
    return false;
  }
  return true;
}

async function refreshList() {
  const list = await api('GET', '/api/instances');
  ui.list = Array.isArray(list) ? list : [];
}

async function refreshResources() {
  const r = await api('GET', '/api/resources/healthcare');
  ui.resources.healthcare = Array.isArray(r) ? r : [];
}

function remember(result, instanceId, title) {
  if (result.instance) ui.instances[result.instance.id] = result.instance;
  if (instanceId) {
    ui.banners[instanceId] = { ok: !!result.ok, warning: !!result.warning, message: result.message || '' };
    ui.changed[instanceId] = new Set((result.changes || []).map((c) => c.task));
  }
  ui.last = {
    title: title,
    ok: !!result.ok,
    warning: !!result.warning,
    message: result.message || '',
    trace: result.trace || [],
    console: result.console || [],
    isolation: null,
  };
}

/** Re-reads every OTHER instance of the industry from the engine and reports whether any task changed. */
async function isolationCheck(industry, actedOn, before) {
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

function statesBefore(industry, exceptId) {
  const before = {};
  ui.list.filter((s) => s.industry === industry && s.id !== exceptId).forEach((s) => {
    const view = ui.instances[s.id];
    if (view) before[s.id] = Object.fromEntries(tasksOf(view.tree).map((t) => [t.id, t.state]));
  });
  return before;
}

async function run(instanceId, title, call) {
  if (ui.busy) return;
  ui.busy = true;
  const view = ui.instances[instanceId];
  const industry = view ? view.industry : ui.tab;
  const before = statesBefore(industry, instanceId);
  const result = await call();
  remember(result, instanceId, title);
  ui.last.isolation = await isolationCheck(industry, instanceId, before);
  await refreshList();
  if (industry === 'healthcare') await refreshResources();
  ui.busy = false;
  render();
}

function taskAction(instanceId, taskId, action) {
  const key = instanceId + '/' + taskId;
  const body = {};
  if (action === 'assign') body.participant = ui.picks[key] || '';
  if (action === 'complete') body.outcome = ui.outcomes[key] || '';
  const view = ui.instances[instanceId];
  const task = view && findTaskView(view, taskId);
  const title = `${action === 'assign' ? 'assignTo' : action}() on ${task ? task.name : taskId} (${instanceId})`;
  return run(instanceId, title, () =>
    api('POST', `/api/instances/${encodeURIComponent(instanceId)}/tasks/${encodeURIComponent(taskId)}/${action}`, body));
}

function instanceOp(instanceId, op) {
  const labels = { start: 'Start instance', undo: 'Undo last action', snapshot: 'Take snapshot', restore: 'Restore latest snapshot' };
  return run(instanceId, `${labels[op]} (${instanceId})`, () =>
    api('POST', `/api/instances/${encodeURIComponent(instanceId)}/${op}`));
}

async function createInstance(industry) {
  const meta = industryMeta(industry);
  const subject = ui.subject[industry] || (meta.subjects[0] && meta.subjects[0].id);
  if (ui.busy) return;
  ui.busy = true;
  const result = await api('POST', '/api/instances', { industry, subject });
  if (result.instance) {
    const id = result.instance.id;
    remember(result, id, `Create instance for ${result.instance.subject.name}`);
    const panes = ui.panes[industry];
    const slot = panes[0] === null ? 0 : panes[1] === null ? 1 : 0;
    panes[slot] = id;
  } else {
    ui.last = { title: 'Create instance', ok: false, message: result.message, trace: [], console: [], isolation: null };
  }
  await refreshList();
  ui.busy = false;
  render();
}

async function resetDemo() {
  if (!window.confirm('Reset the demo? Both scenarios are rebuilt and every instance is discarded.')) return;
  const result = await api('POST', '/api/reset');
  ui.panes = { education: [null, null], healthcare: [null, null] };
  ui.instances = {};
  ui.selected = {};
  ui.banners = {};
  ui.changed = {};
  ui.last = { title: 'Reset demo', ok: !!result.ok, message: result.message, trace: [], console: [], isolation: null };
  await loadMeta();
  await refreshList();
  await refreshResources();
  render();
}

async function openInstance(id, slot) {
  const view = await api('GET', '/api/instances/' + encodeURIComponent(id));
  if (!view || !view.tree) return;
  ui.instances[id] = view;
  const panes = ui.panes[view.industry];
  if (panes[1 - slot] === id) panes[1 - slot] = null;
  panes[slot] = id;
  render();
}

// ---------------------------------------------------------------- rendering

function render() {
  renderTabs();
  const app = document.getElementById('app');
  app.innerHTML = ui.tab === 'engine' ? renderEngine() : renderIndustry(ui.tab);
}

function renderTabs() {
  const tabs = [['education', 'Education'], ['healthcare', 'Healthcare'], ['engine', 'Generic engine vs industry-specific']];
  document.getElementById('tabs').innerHTML = tabs.map(([key, label]) =>
    `<button class="tab" type="button" role="tab" data-act="tab" data-tab="${key}" aria-selected="${ui.tab === key}">${label}</button>`).join('');
}

function renderIndustry(key) {
  const meta = industryMeta(key);
  if (!meta) return '<p class="loading">Loading...</p>';
  const panes = ui.panes[key].filter((id) => id && ui.instances[id]);
  return `
  <div class="industry-layout">
    <aside>
      ${renderStartCard(meta)}
      ${renderInstanceList(key)}
      ${renderChain(meta)}
      ${key === 'healthcare' ? renderResources() : ''}
    </aside>
    <section>
      ${panes.length ? `<div class="panes">${ui.panes[key].map((id, slot) =>
        id && ui.instances[id] ? renderPane(ui.instances[id], slot) : '').join('')}</div>`
        : `<div class="card"><h2>No instance open</h2><p class="muted">Pick ${/^[aeiou]/.test(meta.noun) ? 'an' : 'a'} ${esc(meta.noun)} on the left and press
           <b>Create instance</b>. Create two and use <b>Open beside</b> to compare them side by side.</p></div>`}
    </section>
    <aside class="side-right">${renderPatterns()}</aside>
  </div>`;
}

function renderStartCard(meta) {
  const chosen = ui.subject[meta.key] || (meta.subjects[0] && meta.subjects[0].id);
  const subject = meta.subjects.find((s) => s.id === chosen) || meta.subjects[0];
  return `
  <div class="card">
    <h2>${esc(meta.title)}</h2>
    <p class="small muted">${esc(meta.description)}</p>
    <label class="small" for="subject-${meta.key}"><b>Start a new instance</b> - choose ${/^[aeiou]/.test(meta.noun) ? 'an' : 'a'} ${esc(meta.noun)}</label>
    <div class="stack">
      <select id="subject-${meta.key}" data-act="pick-subject" data-industry="${meta.key}">
        ${meta.subjects.map((s) => `<option value="${esc(s.id)}" ${s.id === chosen ? 'selected' : ''}>${esc(s.name)}</option>`).join('')}
      </select>
      <div class="small muted">${esc(subject ? subject.summary : '')}</div>
      <button class="btn primary" type="button" data-act="create" data-industry="${meta.key}">Create instance</button>
      <div class="note">Clones the "${esc(meta.definitionName)}" definition built by ${esc(meta.director)}.</div>
    </div>
  </div>`;
}

function renderInstanceList(key) {
  const items = ui.list.filter((s) => s.industry === key);
  const open = ui.panes[key];
  return `
  <div class="card">
    <h2>Instances (${items.length})</h2>
    ${items.length ? items.map((s) => {
      const total = s.tasks || 1;
      const bar = STATES.map((st) => s.counts[st] ? `<span class="bg-${st}" style="width:${(100 * s.counts[st] / total).toFixed(1)}%" title="${st}: ${s.counts[st]}"></span>` : '').join('');
      return `
      <div class="inst-card ${open.includes(s.id) ? 'open' : ''}">
        <div class="row"><span class="title">${esc(s.id)}</span><span class="status status-${esc(s.status)}">${esc(s.status)}</span></div>
        <div class="small">${esc(s.subject)}</div>
        <div class="bar">${bar}</div>
        ${s.outcome ? `<div class="small muted">${esc(s.outcome)}</div>` : ''}
        <div class="row">
          <button class="btn small" type="button" data-act="open" data-inst="${esc(s.id)}" data-slot="0">Open</button>
          <button class="btn small" type="button" data-act="open" data-inst="${esc(s.id)}" data-slot="1">Open beside</button>
        </div>
      </div>`;
    }).join('') : '<p class="empty small">None yet.</p>'}
  </div>`;
}

function renderChain(meta) {
  const fmt = (v) => meta.key === 'healthcare' ? 'R' + Number(v).toLocaleString('en-ZA') : String(v);
  return `
  <div class="card">
    <h3>Approval chain for ${esc(meta.chainTask)}</h3>
    <div class="chain">
      ${meta.chain.map((l, i) => `${i ? '<span class="arrow">&rarr;</span>' : ''}
        <span class="link"><b>${esc(l.role)}</b>${l.final ? 'final, ' : ''}limit ${esc(fmt(l.limit))}</span>`).join('')}
    </div>
    <p class="note">Each handler approves if the request value is within its limit, otherwise passes it on.
    The final handler rejects anything over its limit.</p>
  </div>`;
}

function renderResources() {
  const slots = ui.resources.healthcare || [];
  return `
  <div class="card">
    <h3>Doctor slots (engine Resources)</h3>
    ${slots.map((r) => `
      <div class="slot">
        <span><b>${esc(r.name)}</b><br><span class="note">${esc(r.available)} of ${esc(r.capacity)} places free</span></span>
        <span style="text-align:right">
          <span class="${r.isFree ? 'free' : 'full'}">${r.isFree ? 'isFree() = true' : 'FULL'}</span><br>
          <span class="note">${r.grantedTo.length ? 'held by ' + r.grantedTo.map((g) => esc(g.instance)).join(', ') : 'no bookings'}</span>
        </span>
      </div>`).join('')}
    <p class="note">Live from the engine: ResourceCoordinator::requestResource() takes a place, release() gives it back.
    The web layer does not count places.</p>
  </div>`;
}

function renderPane(inst, slot) {
  const meta = industryMeta(inst.industry);
  const banner = ui.banners[inst.id];
  const facts = Object.entries(inst.data)
    .filter(([k]) => !k.startsWith('resourceBy.') && !k.startsWith('refused.') && k !== 'name' && k !== 'outcome')
    .map(([k, v]) => `<span class="fact"><b>${esc(k)}</b> ${esc(v)}</span>`).join('');
  const availableChips = inst.available.map((id) => {
    const t = findTaskView(inst, id);
    return `<button class="chip-btn" type="button" data-act="select" data-inst="${esc(inst.id)}" data-task="${esc(id)}">${esc(t ? t.name : id)}</button>`;
  }).join('');

  return `
  <article class="card pane">
    <div class="pane-head">
      <div class="row">
        <h2>${esc(inst.id)} &middot; ${esc(inst.subject.name)}</h2>
        <span class="status status-${esc(inst.status)}">${esc(inst.status)}</span>
        <span class="spacer"></span>
        <button class="btn small ghost" type="button" data-act="close" data-industry="${esc(inst.industry)}" data-slot="${slot}" title="Close this pane">Close</button>
      </div>
      <div class="small muted">${esc(inst.subject.summary)}</div>
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
      ${renderNode(inst, inst.tree, meta, true)}
      <p class="note">"Available work" is what the engine's <code>ReadyTaskIterator</code> returns.</p>
      <details ${inst.audit.length > 3 ? 'open' : ''}>
        <summary>Audit trail - AuditLogger (${inst.audit.length})</summary>
        ${renderHistory(inst)}
      </details>
      ${inst.snapshots.length ? `<p class="note">Snapshots: ${inst.snapshots.map(esc).join(' | ')}</p>` : ''}
    </div>
  </article>`;
}

function renderNode(inst, node, meta, isRoot) {
  if (node.type === 'stage') {
    return `<div class="stage ${isRoot ? 'root' : ''}">
      ${isRoot ? '' : `<div class="stage-name">${esc(node.name)}</div>`}
      ${node.children.map((c) => renderNode(inst, c, meta, false)).join('')}
    </div>`;
  }
  const t = node;
  const selected = ui.selected[inst.id] === t.id;
  const changed = ui.changed[inst.id] && ui.changed[inst.id].has(t.id);
  const who = t.assignee ? `${esc(t.assignee.name)}` : '<span class="empty">unassigned</span>';
  const extra = [];
  if (t.requestValue !== undefined) extra.push(`request value ${esc(meta.key === 'healthcare' ? 'R' + t.requestValue : t.requestValue)}`);
  if (t.stepSucceeds === false) extra.push('<b>automated step will fail</b>');
  if (t.resourceHeldWith) extra.push(`slot granted with ${esc(t.resourceHeldWith)}`);
  else if (t.resourceRefusedBy && (t.state === 'Assigned' || t.state === 'Escalated')) extra.push(`slot REFUSED by ${esc(t.resourceRefusedBy)}`);
  if (t.prerequisites.length) extra.push('after ' + t.prerequisites.map((p) => { const pt = findTaskView(inst, p); return esc(pt ? pt.name : p); }).join(' + '));
  const decorated = (t.decorators || []).map((d) => `<span class="tag tag-engine" title="wrapped by a real decorator">${esc(d)}</span>`).join('');
  return `
    <div class="task ${selected ? 'selected' : ''} ${changed ? 'changed' : ''}" data-act="select" data-inst="${esc(inst.id)}" data-task="${esc(t.id)}" role="button" tabindex="0">
      ${stateChip(t.state)}
      <div><div class="name">${esc(t.name)} <span class="kind">${esc(kindLabel(t.kind))}</span>${decorated}</div>
        <div class="meta"><span class="mono">${esc(t.id)}</span> &middot; ${who}${extra.length ? ' &middot; ' + extra.join(' &middot; ') : ''}</div></div>
    </div>
    ${selected ? renderActions(inst, t, meta) : ''}`;
}

function renderActions(inst, t, meta) {
  const key = inst.id + '/' + t.id;
  const people = meta.participants.slice().sort((a, b) => (b.role === t.roleHint) - (a.role === t.roleHint));
  if (!ui.picks[key]) ui.picks[key] = people[0] ? people[0].id : '';
  const outcomes = t.outcomes || [];
  if (outcomes.length && !ui.outcomes[key]) ui.outcomes[key] = (inst.data.suggestedUrgency && outcomes.includes(inst.data.suggestedUrgency)) ? inst.data.suggestedUrgency : outcomes[0];
  return `
  <div class="actions">
    <div class="help">${esc(t.help || '')} <br>Usually done by: <b>${esc(t.roleHint || '-')}</b>.
      Assignment is manual: the Strategy classes do not compile yet.</div>
    <div class="row">
      <select data-act="pick-person" data-key="${esc(key)}" aria-label="Participant">
        ${people.map((p) => `<option value="${esc(p.id)}" ${ui.picks[key] === p.id ? 'selected' : ''}>${esc(p.name)} - ${esc(p.role)}${p.slots >= 0 ? ` (slots ${p.slots})` : ''}</option>`).join('')}
      </select>
      <button class="btn small primary" type="button" data-act="task" data-action="assign" data-inst="${esc(inst.id)}" data-task="${esc(t.id)}">assignTo</button>
    </div>
    ${outcomes.length ? `<div class="row" style="margin-top:.4rem"><label class="small" for="out-${esc(key)}">${esc(t.outcomeQuestion)} (used by complete):</label>
      <select id="out-${esc(key)}" data-act="pick-outcome" data-key="${esc(key)}">
        ${outcomes.map((o) => `<option value="${esc(o)}" ${ui.outcomes[key] === o ? 'selected' : ''}>${esc(o)}</option>`).join('')}
      </select></div>` : ''}
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
  const label = { engine: 'engine', temporary: 'temporary web stand-in', scenario: 'industry rule', pending: 'not yet integrated' }[kind] || kind;
  return `<span class="tag tag-${esc(kind)}">${esc(label)}</span>`;
}

function renderPatterns() {
  const last = ui.last;
  const legend = `<div class="legend">${kindTag('engine')}${kindTag('scenario')}${kindTag('temporary')}${kindTag('pending')}</div>`;
  if (!last) {
    return `<div class="card"><h2>Patterns in action</h2>${legend}
      <p class="muted small">After every action this panel lists, in order, the classes that took part and the
      pattern each one plays. Steps done by a temporary web stand-in or not yet integrated are labelled as such.</p></div>`;
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
        ${s.kind === 'pending' ? '<span class="tag tag-pending">not yet integrated</span>' : ''}
        ${s.detail ? `<div class="detail">${esc(s.detail)}</div>` : ''}
      </li>`).join('')}</ol>` : '<p class="empty small">No engine steps.</p>'}
    <details ${last.console.length ? 'open' : ''}>
      <summary>Engine console output (${last.console.length})</summary>
      <pre class="console">${last.console.length ? last.console.map(esc).join('\n') : '(the engine printed nothing)'}</pre>
    </details>
  </div>`;
}

function miniTree(node) {
  if (node.type === 'task') return `<li>${esc(node.name)} <span class="kind">${esc(node.kind)}</span></li>`;
  return `<li><b>${esc(node.name)}</b><ul>${node.children.map(miniTree).join('')}</ul></li>`;
}

function renderEngine() {
  const meta = ui.meta;
  const inds = meta.industries;
  return `
  <div class="card">
    <h2>One generic engine, two industries</h2>
    <p class="muted">Both industries run on the same compiled engine classes from <code>include/</code> and <code>src/</code>.
    Only the definitions, people, rules and data differ (<code>web/scenarios/</code>); the spec permits
    industry-specific definitions, rules and data.</p>
  </div>
  <div class="engine-grid">
    <div class="card">
      <h2>Generic engine classes reused unchanged by both industries</h2>
      <ul class="classlist">${GENERIC_CLASSES.map((c) => `<li>${c}</li>`).join('')}</ul>
    </div>
    ${inds.map((ind) => `
    <div class="card">
      <h2>Industry-specific: ${esc(ind.title)}</h2>
      <p class="small"><b>Director:</b> <code>${esc(ind.director)}</code>${ind.key === 'education' ? ' (src/, the team\'s)' : ' (web/scenarios/)'}
      &middot; <b>Rules and data:</b> <code>${ind.key === 'education' ? 'EducationScenario' : 'HealthcareScenario'}</code></p>
      <p class="small"><b>People:</b> ${ind.participants.map((p) => esc(p.name) + ' (' + esc(p.role) + ')').join(', ')}</p>
      <p class="small"><b>Approval chain:</b> ${ind.chain.map((l) => esc(l.role)).join(' &rarr; ')}</p>
      <div class="mini-tree"><ul>${miniTree(ind.definition)}</ul></div>
      <p class="note">Decorated through the builder: ${ind.decorated.map(esc).join(', ')} (AuditDecorator).</p>
    </div>`).join('')}
  </div>
  <div class="card">
    <h2>Pattern status in this build</h2>
    <div class="table-wrap"><table class="ptable">
      <thead><tr><th>Pattern</th><th>Engine classes</th><th>Status</th><th>In the demo</th></tr></thead>
      <tbody>${PATTERNS.map(([p, c, s, n]) => `<tr><td><b>${esc(p)}</b></td><td><code>${esc(c)}</code></td>
        <td><span class="pill pill-${s}">${{ working: 'working', partial: 'partial', temporary: 'temporary stand-in', missing: 'not yet integrated' }[s]}</span></td>
        <td>${esc(n)}</td></tr>`).join('')}</tbody>
    </table></div>
    <p class="note">Strategy: ${esc(meta.strategy)}</p>
  </div>
  <div class="engine-grid">
    <div class="card">
      <h2>Temporary web stand-ins</h2>
      <table class="ptable"><thead><tr><th>File (web/)</th><th>Stands in for</th><th>Removed when</th></tr></thead>
      <tbody>${TEMPORARY.map(([f, r, m]) => `<tr><td><code>${esc(f)}</code></td><td>${esc(r)}</td><td>${esc(m)}</td></tr>`).join('')}</tbody></table>
      <p class="note">Details and swap steps: <code>web/INTEGRATION.md</code>.</p>
    </div>
    ${inds.map((ind) => `
    <div class="card">
      <h2>How the ${esc(ind.key)} definition was built</h2>
      <ol class="trace">${ind.buildTrace.map((s) => `<li class="k-${esc(s.kind)}"><span class="actor">${esc(s.actor)}</span>
        <span class="tag tag-${esc(s.kind)}">${esc(s.pattern)}</span>${s.kind === 'temporary' ? '<span class="tag tag-temporary">(temporary web stand-in)</span>' : ''}${s.kind === 'pending' ? '<span class="tag tag-pending">not yet integrated</span>' : ''}
        <div class="detail">${esc(s.detail)}</div></li>`).join('')}</ol>
    </div>`).join('')}
  </div>`;
}

// ---------------------------------------------------------------- events

document.addEventListener('click', (event) => {
  const el = event.target.closest('[data-act]');
  if (!el) return;
  const act = el.dataset.act;
  if (act === 'tab') { ui.tab = el.dataset.tab; render(); }
  else if (act === 'create') createInstance(el.dataset.industry);
  else if (act === 'open') openInstance(el.dataset.inst, Number(el.dataset.slot));
  else if (act === 'close') { ui.panes[el.dataset.industry][Number(el.dataset.slot)] = null; render(); }
  else if (act === 'select') {
    if (event.target.closest('.actions')) return;
    ui.selected[el.dataset.inst] = ui.selected[el.dataset.inst] === el.dataset.task ? null : el.dataset.task;
    render();
  }
  else if (act === 'task') taskAction(el.dataset.inst, el.dataset.task, el.dataset.action);
  else if (act === 'op') instanceOp(el.dataset.inst, el.dataset.op);
});

document.addEventListener('keydown', (event) => {
  const el = event.target.closest && event.target.closest('[data-act="select"]');
  if (el && (event.key === 'Enter' || event.key === ' ')) { event.preventDefault(); el.click(); }
});

document.addEventListener('change', (event) => {
  const el = event.target;
  if (!el.dataset) return;
  if (el.dataset.act === 'pick-subject') { ui.subject[el.dataset.industry] = el.value; render(); }
  else if (el.dataset.act === 'pick-person') ui.picks[el.dataset.key] = el.value;
  else if (el.dataset.act === 'pick-outcome') ui.outcomes[el.dataset.key] = el.value;
});

document.getElementById('resetDemo').addEventListener('click', resetDemo);
document.getElementById('themeToggle').addEventListener('click', () => {
  const root = document.documentElement;
  const dark = root.dataset.theme ? root.dataset.theme === 'dark' : window.matchMedia('(prefers-color-scheme: dark)').matches;
  root.dataset.theme = dark ? 'light' : 'dark';
});

(async function init() {
  if (!(await loadMeta())) return;
  await refreshList();
  await refreshResources();
  render();
})();
