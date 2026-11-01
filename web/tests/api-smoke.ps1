# End-to-end smoke test of the web API against the REAL engine.
# Start the server first (.\web\run.ps1 -NoBrowser), then:   .\web\tests\api-smoke.ps1
# Covers: templates, generic scenarios, a brand-new scenario built through the API, invalid specs,
# rule-based and runtime-chosen decisions, the approval chain's generic levels, and the completion
# and exception paths of both templates. Exit code 0 = all checks passed.
param([string]$Base = 'http://localhost:8080')

$ErrorActionPreference = 'Stop'
$script:passed = 0
$script:failed = 0

function Check([bool]$condition, [string]$what) {
    if ($condition) { $script:passed++; Write-Host "  PASS  $what" -ForegroundColor Green }
    else { $script:failed++; Write-Host "  FAIL  $what" -ForegroundColor Red }
}

function Call([string]$method, [string]$path, $body = $null) {
    $args = @{ Method = $method; Uri = "$Base$path"; ContentType = 'application/json' }
    if ($body -ne $null) { $args.Body = ($body | ConvertTo-Json -Compress -Depth 30) }
    try { return Invoke-RestMethod @args }
    catch {
        # 4xx/5xx: the server still sends a JSON {ok, message, errors} body.
        if ($_.ErrorDetails -and $_.ErrorDetails.Message) { return ($_.ErrorDetails.Message | ConvertFrom-Json) }
        throw
    }
}

$script:scenarios = @{}
function Load-Scenarios { (Call GET '/api/scenarios').scenarios | ForEach-Object { $script:scenarios[$_.key] = $_ } }
function Seed($key, $label) { ($script:scenarios[$key].seeds | Where-Object { $_.label -eq $label }).data }
function New-Instance($key, $inputs) { Call POST '/api/instances' @{ scenario = $key; inputs = $inputs } }
function New-FromSeed($key, $label) { (New-Instance $key (Seed $key $label)).instance.id }
function Act($id, $task, $action, $extra = @{}) { Call POST "/api/instances/$id/tasks/$task/$action" $extra }
function Op($id, $op) { Call POST "/api/instances/$id/$op" }
function View($id) { Call GET "/api/instances/$id" }

function Tasks($node) {
    if ($node.type -eq 'task') { return ,$node }
    $all = @()
    foreach ($c in $node.children) { $all += Tasks $c }
    return $all
}
function TaskOf($view, $task) { (Tasks $view.tree) | Where-Object { $_.id -eq $task } }
function StateOf($view, $task) { (TaskOf $view $task).state }
function Traced($result, $text) { @($result.trace | Where-Object { ($_.actor + ' ' + $_.detail) -like "*$text*" }).Count -gt 0 }

function Do-Manual($id, $task, $who, $branch = '') {
    $null = Act $id $task 'assign' @{ participant = $who }
    $null = Act $id $task 'start'
    return Act $id $task 'complete' @{ branch = $branch }
}
function Do-Execute($id, $task, $who) {
    $null = Act $id $task 'assign' @{ participant = $who }
    $null = Act $id $task 'start'
    return Act $id $task 'execute'
}

Write-Host "Templates and scenarios"
$r = Call POST '/api/reset'
Check ($r.ok -and $r.templateErrors.Count -eq 0) 'reset reloads the templates without errors'
$t = Call GET '/api/templates'
Check ($t.templates.Count -ge 2 -and ($t.templates | ForEach-Object { $_.name }) -contains 'University admission' -and ($t.templates | ForEach-Object { $_.name }) -contains 'Patient booking') 'GET /api/templates lists both template files'
Load-Scenarios
$edu = $script:scenarios['university-admission']
$hc = $script:scenarios['patient-booking']
Check ($edu -ne $null -and $hc -ne $null) 'each template became a scenario (built by GenericDirector, registered in DefinitionRegistry)'
Check (@($edu.definition.children | Where-Object { $_.name -eq 'Assessment' }).children.name -contains 'Admission decision') 'nested sub-stage (Composite) shown with its name'
Check ($edu.chains[0].levels.Count -eq 3 -and $edu.chains[0].levels[0].label -eq 'Approver level 1' -and $edu.chains[0].levels[2].final) 'approval chain has generic levels ending in a final approver'

# ------------------------------------------------------------------ University admission (template)
Write-Host "`nUniversity admission - completion path (Thandiwe)"
$a = New-FromSeed 'university-admission' 'Thandiwe Mokoena'
$b = New-FromSeed 'university-admission' 'Thandiwe Mokoena'
Check ($a -eq 'EDU-1' -and $b -eq 'EDU-2') 'two independent instances created from a seed case'
$s = Op $a 'start'
Check ($s.ok -and (StateOf $s.instance 'SubmitApplication') -eq 'Assigned' -and (StateOf $s.instance 'CheckDocuments') -eq 'Created' -and (Traced $s 'WorkflowManager::start')) 'start: WorkflowManager::start opens only Submit Application'
$bad = Act $a 'CheckDocuments' 'complete'
Check (-not $bad.ok -and $bad.message -like '*Created*') 'complete() on a Created task is refused (WorkflowManager: prerequisite not met)'
$bad = Act $a 'CheckDocuments' 'makeAvailable'
Check (-not $bad.ok -and $bad.message -like '*Dependency*') 'makeAvailable() refused by the Mediator before the prerequisite completes'
Check ((TaskOf $s.instance 'SubmitApplication').assignee.name -like 'Applicant*' -and (Traced $s 'RoleBasedAssignment::route')) 'start routes it: TaskRouter + RoleBasedAssignment chose the applicant'
$null = Act $a 'SubmitApplication' 'start'
$r = Act $a 'SubmitApplication' 'complete'
Check ((StateOf $r.instance 'CheckDocuments') -eq 'Available' -and (Traced $r 'WorkflowManager::complete')) 'WorkflowManager::complete unlocks Check Documents (Mediator)'
$r = Act $a 'CheckDocuments' 'autoAssign'
Check ($r.ok -and (Traced $r 'WorkloadAssignment::route')) 'auto-assign with WorkloadAssignment'
$null = Act $a 'CheckDocuments' 'start'
$r = Act $a 'CheckDocuments' 'complete'
Check ((StateOf $r.instance 'VerifyPersonalDetails') -eq 'Available' -and (StateOf $r.instance 'VerifyResults') -eq 'Available') 'fork: both verifications unlock'
$r = Do-Execute $a 'VerifyPersonalDetails' 'system'
Check ((StateOf $r.instance 'VerifyPersonalDetails') -eq 'Completed' -and (StateOf $r.instance 'FacultyChecksRequirements') -eq 'Created') 'automated step completes; join still waits'
$r = Do-Execute $a 'VerifyResults' 'system'
Check ((StateOf $r.instance 'FacultyChecksRequirements') -eq 'Available' -and (Traced $r 'made available: Faculty')) 'join: DependencyActivator unlocks the faculty check after both branches'
$null = Do-Manual $a 'FacultyChecksRequirements' 'botha'
$r = Act $a 'ApplicationDecision' 'assign' @{ participant = 'botha' }
Check ((TaskOf $r.instance 'ApplicationDecision').decorators -contains 'AuditDecorator' -and (Traced $r 'audit line')) 'AuditDecorator wraps the decision and audits the assignment'
$null = Act $a 'ApplicationDecision' 'start'
$r = Act $a 'ApplicationDecision' 'execute'
Check ((StateOf $r.instance 'ApplicationDecision') -eq 'Completed' -and $r.message -like '*Approver level 1 approved*') 'risk 15: approver level 1 approves'
$r = Do-Execute $a 'NotifyApplicant' 'system'
Check ($r.instance.status -eq 'Finished' -and $r.instance.outcome -eq 'Admitted to BSc Computer Science') 'notified; Finished; outcome from the spec template'
Check (@($r.instance.audit | Where-Object { $_ -like '*ITEM_COMPLETED*' }).Count -eq 7) 'AuditLogger recorded all 7 completions'
$v = View $b
Check ($v.status -eq 'Created' -and $v.audit.Count -eq 1) 'isolation: EDU-2 untouched'

Write-Host "`nUniversity admission - undo, snapshot and exception paths"
$null = Op $b 'start'
$r = Act $b 'SubmitApplication' 'start'
$r = Op $b 'undo'
Check ($r.ok -and (StateOf $r.instance 'SubmitApplication') -eq 'Assigned' -and (Traced $r 'WorkflowManager::undo')) 'WorkflowManager::undo pops StartCommand'
$null = Op $b 'snapshot'
$null = Do-Manual $b 'SubmitApplication' 'applicant'
$r = Op $b 'restore'
Check ($r.ok -and (StateOf $r.instance 'SubmitApplication') -eq 'Assigned' -and (StateOf $r.instance 'CheckDocuments') -eq 'Created' -and (Traced $r 'WorkflowManager::restoreSnapshot')) 'restore snapshot rolls the tree back'

$k = New-FromSeed 'university-admission' 'Kagiso Sithole'
$null = Op $k 'start'
$null = Do-Manual $k 'SubmitApplication' 'applicant'
$null = Act $k 'CheckDocuments' 'assign' @{ participant = 'khumalo' }
$null = Act $k 'CheckDocuments' 'start'
$r = Act $k 'CheckDocuments' 'reject'
Check ((StateOf $r.instance 'CheckDocuments') -eq 'Rejected' -and $r.instance.data.documents -like 'missing*') 'documents missing: rejected (onReject sets case data)'
$r = Do-Manual $k 'CheckDocuments' 'steyn'
Check ((StateOf $r.instance 'VerifyResults') -eq 'Available') 'loop back: reassigned, then the fork opens'

$sp = New-FromSeed 'university-admission' 'Sipho Dlamini'
$null = Op $sp 'start'
$null = Do-Manual $sp 'SubmitApplication' 'applicant'
$null = Do-Manual $sp 'CheckDocuments' 'khumalo'
$null = Do-Execute $sp 'VerifyPersonalDetails' 'system'
$null = Do-Execute $sp 'VerifyResults' 'system'
$null = Do-Manual $sp 'FacultyChecksRequirements' 'botha'
$r = Do-Execute $sp 'ApplicationDecision' 'botha'
Check ((StateOf $r.instance 'ApplicationDecision') -eq 'Rejected' -and $r.message -like '*Approver level 1 -> Approver level 2 -> Final approver; Final approver rejected*') 'risk 91: escalates through every generic level; the final approver rejects'
Check ((StateOf $r.instance 'NotifyApplicant') -eq 'Available') 'onReject unlocks Notify Applicant'

$ai = New-FromSeed 'university-admission' 'Aisha Patel'
Check ((TaskOf (View $ai) 'VerifyResults').stepSucceeds -eq $false) 'success rule curriculum == NSC fails for Cambridge: setStepSucceeds(false)'
$null = Op $ai 'start'
$null = Do-Manual $ai 'SubmitApplication' 'applicant'
$null = Do-Manual $ai 'CheckDocuments' 'khumalo'
$r = Do-Execute $ai 'VerifyResults' 'system'
Check ((StateOf $r.instance 'VerifyResults') -eq 'InProgress' -and $r.warning) 'automated results check fails: stays InProgress'
$null = Act $ai 'VerifyResults' 'escalate'
$r = Do-Manual $ai 'VerifyResults' 'khumalo'
Check ((StateOf $r.instance 'VerifyResults') -eq 'Completed') 'escalated, reassigned and completed by hand'

$p = New-FromSeed 'university-admission' 'Pieter van Wyk'
$null = Op $p 'start'
$null = Do-Manual $p 'SubmitApplication' 'applicant'
$null = Do-Manual $p 'CheckDocuments' 'khumalo'
$null = Do-Execute $p 'VerifyPersonalDetails' 'system'
$null = Do-Execute $p 'VerifyResults' 'system'
$null = Do-Manual $p 'FacultyChecksRequirements' 'botha'
$r = Do-Execute $p 'ApplicationDecision' 'botha'
Check ($r.message -like '*Approver level 2 approved*') 'risk 48: approver level 2 approves'

# ------------------------------------------------------------------ Patient booking (template)
Write-Host "`nPatient booking - runtime-chosen decision and live contention"
$j = New-FromSeed 'patient-booking' 'Johan Pretorius'
$null = Op $j 'start'
$null = Do-Manual $j 'RegisterPatient' 'mthembu'
$r = Do-Execute $j 'VerifyMedicalAid' 'system'
Check ((StateOf $r.instance 'VerifyMedicalAid') -eq 'Completed') 'active medical aid: automated step completes'
$bad = Act $j 'BookAppointment' 'makeAvailable'
Check (-not $bad.ok) 'no booking before triage (Mediator refuses)'
$v = View $j
Check ((TaskOf $v 'TriageAssessment').decision.mode -eq 'manual' -and (TaskOf $v 'TriageAssessment').decision.suggested -eq 'routine') 'triage is a runtime decision; the rule suggests routine'
$r = Do-Manual $j 'TriageAssessment' 'mahlangu' 'routine'
Check ((StateOf $r.instance 'BookAppointment') -eq 'Available' -and (StateOf $r.instance 'EmergencyConsultation') -eq 'Cancelled' -and $r.message -like '*chosen by the user*') 'runtime choice "routine": emergency branch cancelled'
$r = Act $j 'BookAppointment' 'assign' @{ participant = 'naidoo' }
Check ($r.warning -and $r.message -like '*refused*') 'Dr Naidoo has no free place: ResourceCoordinator refuses'
$r = Act $j 'BookAppointment' 'start'
Check (-not $r.ok) 'a task that needs a resource cannot start without one'
$null = Act $j 'BookAppointment' 'escalate'
$r = Act $j 'BookAppointment' 'assign' @{ participant = 'mokoena' }
Check ($r.message -like '*Resource granted*') 'reassigned to Dr Mokoena: granted'
$res = Call GET '/api/resources/patient-booking'
Check ((($res | Where-Object { $_.id -eq 'mokoenaSlots' }).available) -eq 1) 'Dr Mokoena: 1 of 2 places free'
$null = Act $j 'BookAppointment' 'start'
$null = Act $j 'BookAppointment' 'complete'
$null = Do-Execute $j 'SendReminder' 'system'
$null = Do-Manual $j 'GPConsultation' 'mokoena'
$res = Call GET '/api/resources/patient-booking'
Check ((($res | Where-Object { $_.id -eq 'mokoenaSlots' }).available) -eq 2) 'GP consultation onComplete releases the slot'
$null = Act $j 'SpecialistReferral' 'assign' @{ participant = 'mahlangu' }
$r = Act $j 'SpecialistReferral' 'start'
Check (Traced $r 'SpecialistReferral: start succeeded') 'AuditDecorator audits the referral'
$r = Act $j 'SpecialistReferral' 'execute'
Check ($r.message -like '*Approver level 1 approved*' -and $r.instance.status -eq 'Finished') 'R450: approver level 1 approves; visit Finished'

$l = New-FromSeed 'patient-booking' 'Lerato Molefe'
$null = Op $l 'start'
$null = Do-Manual $l 'RegisterPatient' 'mthembu'
$null = Do-Execute $l 'VerifyMedicalAid' 'system'
Check ((TaskOf (View $l) 'TriageAssessment').decision.suggested -eq 'urgent') 'the rule suggests urgent for Lerato'
$r = Do-Manual $l 'TriageAssessment' 'dube' 'urgent'
Check ((StateOf $r.instance 'EmergencyConsultation') -eq 'Available' -and (StateOf $r.instance 'BookAppointment') -eq 'Cancelled') 'runtime choice "urgent": booking branch cancelled'
$r = Do-Manual $l 'EmergencyConsultation' 'sibiya'
Check ($r.instance.status -eq 'Finished' -and $r.instance.outcome -eq 'Treated by the doctor on duty') 'urgent visit Finished'

$m = New-FromSeed 'patient-booking' 'Mandla Zungu'
$bo = New-FromSeed 'patient-booking' 'Bongani Nkosi'
foreach ($id in @($m, $bo)) {
    $null = Op $id 'start'
    $null = Do-Manual $id 'RegisterPatient' 'mthembu'
    $null = Do-Execute $id 'VerifyMedicalAid' 'system'
    $null = Do-Manual $id 'TriageAssessment' 'mahlangu' 'routine'
}
$r = Act $m 'BookAppointment' 'assign' @{ participant = 'vdmerwe' }
Check ($r.message -like '*Resource granted*') "$m books Dr van der Merwe's only place"
$r = Act $bo 'BookAppointment' 'assign' @{ participant = 'vdmerwe' }
Check ($r.warning -and $r.message -like '*refused*') "$bo is refused: that place is taken (live contention)"
$null = Act $bo 'BookAppointment' 'escalate'
$null = Act $bo 'BookAppointment' 'assign' @{ participant = 'mokoena' }
$null = Act $m 'BookAppointment' 'start'
$null = Act $m 'BookAppointment' 'complete'
$r = Act $m 'GPConsultation' 'cancel'
Check ((StateOf $r.instance 'SpecialistReferral') -eq 'Cancelled' -and $r.instance.outcome -like '*no-show*') 'no-show: onCancel cancels follow-ups'
$res = Call GET '/api/resources/patient-booking'
Check ((($res | Where-Object { $_.id -eq 'vdmerweSlots' }).available) -eq 1) "no-show released Dr van der Merwe's place"
$r = Op $m 'undo'
Check ($r.ok -and (StateOf $r.instance 'SpecialistReferral') -eq 'Created' -and (StateOf $r.instance 'GPConsultation') -eq 'Cancelled') 'undo pops only the last command'
$null = Act $bo 'BookAppointment' 'start'
$null = Act $bo 'BookAppointment' 'complete'
$null = Do-Manual $bo 'GPConsultation' 'mokoena'
$r = Do-Execute $bo 'SpecialistReferral' 'mahlangu'
Check ($r.message -like '*Approver level 1 -> Approver level 2 -> Approver level 3; Approver level 3 approved*') 'R9 800: escalates to approver level 3'

$f = New-FromSeed 'patient-booking' 'Fatima Ebrahim'
$null = Op $f 'start'
$null = Do-Manual $f 'RegisterPatient' 'mthembu'
$r = Do-Execute $f 'VerifyMedicalAid' 'system'
Check ((StateOf $r.instance 'VerifyMedicalAid') -eq 'InProgress' -and $r.message -like '*FAILED*') 'suspended medical aid: automated step fails'
$null = Act $f 'VerifyMedicalAid' 'escalate'
$null = Do-Manual $f 'VerifyMedicalAid' 'mthembu'
$null = Do-Manual $f 'TriageAssessment' 'mahlangu' 'routine'
$null = Act $f 'BookAppointment' 'assign' @{ participant = 'vdmerwe' }
$null = Act $f 'BookAppointment' 'start'
$null = Act $f 'BookAppointment' 'complete'
$null = Do-Manual $f 'GPConsultation' 'vdmerwe'
$r = Do-Execute $f 'SpecialistReferral' 'mahlangu'
Check ((StateOf $r.instance 'SpecialistReferral') -eq 'Rejected' -and $r.message -like '*Final approver rejected*') 'R31 500: the final approver rejects'

# ------------------------------------------------------------------ A brand-new scenario through the API
Write-Host "`nNew scenario from scratch: car insurance claim"
$claim = @{
    name = 'Car insurance claim'; industry = 'Insurance'; idPrefix = 'CLM'; subjectField = 'claimant'
    description = 'Lodge, assess, route by claim value, approve and pay out.'
    inputs = @(
        @{ key = 'claimant'; label = 'Claimant'; type = 'text' },
        @{ key = 'claimValue'; label = 'Claim value (R)'; type = 'number' },
        @{ key = 'policyActive'; label = 'Policy active'; type = 'choice'; options = @('yes', 'no') }
    )
    participants = @(
        @{ id = 'clerk'; name = 'Sam Clerk'; role = 'Claims clerk' },
        @{ id = 'assessor'; name = 'Ava Assessor'; role = 'Assessor' },
        @{ id = 'manager'; name = 'Max Manager'; role = 'Claims manager' },
        @{ id = 'system'; name = 'Payment system'; role = 'System' }
    )
    stages = @(
        @{ id = 'Intake'; name = 'Intake'; tasks = @(
            @{ id = 'LodgeClaim'; name = 'Lodge claim'; type = 'manual'; role = 'Claims clerk'; strategy = 'role' },
            @{ id = 'AssessDamage'; name = 'Assess damage'; type = 'manual'; role = 'Assessor'; dependsOn = @('LodgeClaim'); strategy = 'role' }) },
        @{ id = 'Settlement'; name = 'Settlement'; tasks = @(
            @{ id = 'FastPayout'; name = 'Fast-track payout'; type = 'automated'; role = 'System'; dependsOn = @('AssessDamage')
               successRule = @{ field = 'policyActive'; op = '=='; value = 'yes' }; onComplete = @{ outcome = 'Paid R{claimValue} (fast track)' } })
           stages = @(@{ id = 'FullReview'; name = 'Full review'; tasks = @(
            @{ id = 'ApproveClaim'; name = 'Approve claim'; type = 'approval'; role = 'Claims manager'; dependsOn = @('AssessDamage')
               requestValue = @{ field = 'claimValue' }
               chain = @(@{ label = 'Approver level 1'; role = 'Claims manager'; limit = 100000 }, @{ label = 'Final approver'; role = 'Finance director'; limit = 250000 })
               onReject = @{ outcome = 'Claim declined' } },
            @{ id = 'Payout'; name = 'Pay out'; type = 'automated'; role = 'System'; dependsOn = @('ApproveClaim')
               successRule = @{ field = 'policyActive'; op = '=='; value = 'yes' }; onComplete = @{ outcome = 'Paid R{claimValue}' } }) }) }
    )
    decisions = @(@{ id = 'route'; label = 'Claim route'; after = 'AssessDamage'; mode = 'rule'
        branches = @(@{ id = 'fast'; label = 'Fast track'; tasks = @('FastPayout') }, @{ id = 'full'; label = 'Full review'; tasks = @('ApproveClaim', 'Payout') })
        rules = @(@{ field = 'claimValue'; op = '>'; value = '50000'; branch = 'full' }); default = 'fast' })
}
$r = Call POST '/api/scenarios/validate' $claim
Check ($r.ok) 'the new spec validates'
$r = Call POST '/api/scenarios' $claim
Check ($r.ok -and $r.scenario.key -eq 'car-insurance-claim') 'created through the API: built and registered'
$again = Call POST '/api/scenarios' $claim
Check (-not $again.ok -and ($again.errors -join ' ') -like '*already used*') 'the same name cannot be created twice'

$c1 = (New-Instance 'car-insurance-claim' @{ claimant = 'Nomvula Khumalo'; claimValue = 180000; policyActive = 'yes' }).instance.id
Check ($c1 -eq 'CLM-1') 'instance created with typed input data'
$null = Op $c1 'start'
$null = Do-Manual $c1 'LodgeClaim' 'clerk'
$r = Do-Manual $c1 'AssessDamage' 'assessor'
Check ((StateOf $r.instance 'ApproveClaim') -eq 'Available' -and (StateOf $r.instance 'FastPayout') -eq 'Cancelled' -and $r.message -like '*decided by the rule*') 'rule-based decision: claimValue > 50000 takes the full-review branch'
$r = Do-Execute $c1 'ApproveClaim' 'manager'
Check ($r.message -like '*Approver level 1 -> Final approver; Final approver approved*') 'R180 000 escalates past level 1 to the final approver'
$r = Do-Execute $c1 'Payout' 'system'
Check ($r.instance.status -eq 'Finished' -and $r.instance.outcome -eq 'Paid R180000') 'payout completes; outcome filled from input data'

$c2 = (New-Instance 'car-insurance-claim' @{ claimant = 'Pieter Botha'; claimValue = 12000; policyActive = 'no' }).instance.id
$null = Op $c2 'start'
$null = Do-Manual $c2 'LodgeClaim' 'clerk'
$r = Do-Manual $c2 'AssessDamage' 'assessor'
Check ((StateOf $r.instance 'FastPayout') -eq 'Available' -and (StateOf $r.instance 'ApproveClaim') -eq 'Cancelled') 'R12 000: no rule holds, default fast-track branch'
$r = Do-Execute $c2 'FastPayout' 'system'
Check ((StateOf $r.instance 'FastPayout') -eq 'InProgress' -and $r.warning) 'lapsed policy: the payout step fails'

$c3 = (New-Instance 'car-insurance-claim' @{ claimant = 'Override test'; claimValue = 90000; policyActive = 'yes' }).instance.id
$null = Op $c3 'start'
$null = Do-Manual $c3 'LodgeClaim' 'clerk'
$r = Do-Manual $c3 'AssessDamage' 'assessor' 'fast'
Check ((StateOf $r.instance 'FastPayout') -eq 'Available' -and $r.message -like '*MANUAL OVERRIDE*') 'manual override of a rule-based decision'

Write-Host "`nInvalid specs and errors"
$r = Call POST '/api/scenarios' @{ industry = 'X'; participants = @(); stages = @() }
Check (-not $r.ok -and ($r.errors -join ' ') -like '*name*required*' -and ($r.errors -join ' ') -like '*participant*') 'missing name / participants / stages are reported'
$bad = @{ name = 'Broken'; industry = 'Test'; participants = @(@{ id = 'p'; name = 'P'; role = 'R' })
    stages = @(@{ id = 'S'; tasks = @(
        @{ id = 'A'; type = 'manual'; role = 'R'; dependsOn = @('B') },
        @{ id = 'B'; type = 'manual'; role = 'R'; dependsOn = @('A') },
        @{ id = 'C'; type = 'approval'; role = 'R' },
        @{ id = 'D'; type = 'manual'; role = 'Ghost'; dependsOn = @('Nope') }) }) }
$r = Call POST '/api/scenarios' $bad
$all = $r.errors -join ' | '
Check ($all -like '*cycle*') 'dependency cycle detected'
Check ($all -like '*approval chain*') 'empty approval chain rejected'
Check ($all -like "*unknown task 'Nope'*") 'unknown dependency reported'
Check ($all -like "*role 'Ghost'*") 'unknown role reported'
$r = Call POST '/api/instances' @{ scenario = 'car-insurance-claim'; inputs = @{ claimValue = 'lots' } }
Check (-not $r.ok -and ($r.errors -join ' ') -like '*number*') 'non-numeric input for a number field is rejected'
$r = Call GET '/api/instances/NOPE-1'
Check (-not $r.ok -and $r.message -like '*No instance*') 'unknown instance -> friendly 404'
$r = Act $j 'GPConsultation' 'dance'
Check (-not $r.ok -and $r.message -like '*Unknown action*') 'unknown action -> 400'
$r = Call POST '/api/reset'
Load-Scenarios
Check (-not $script:scenarios.ContainsKey('car-insurance-claim') -or (Call GET '/api/scenarios').scenarios.key -notcontains 'car-insurance-claim') 'reset removes custom scenarios'

Write-Host "`n$($script:passed) passed, $($script:failed) failed"
exit $script:failed
