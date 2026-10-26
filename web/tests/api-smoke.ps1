# End-to-end smoke test of the web API against the REAL engine.
# Start the server first (.\web\run.ps1 -NoBrowser), then:   .\web\tests\api-smoke.ps1
# Walks the completion path and the exception paths of both industries and checks
# what the engine reports. Exit code 0 = all checks passed.
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
    if ($body -ne $null) { $args.Body = ($body | ConvertTo-Json -Compress) }
    try { return Invoke-RestMethod @args }
    catch {
        # 4xx/5xx: the server still sends a JSON {ok, message} body.
        if ($_.ErrorDetails -and $_.ErrorDetails.Message) { return ($_.ErrorDetails.Message | ConvertFrom-Json) }
        throw
    }
}

function New-Instance($industry, $subject) { (Call POST '/api/instances' @{ industry = $industry; subject = $subject }).instance.id }
function Act($id, $task, $action, $extra = @{}) { Call POST "/api/instances/$id/tasks/$task/$action" $extra }
function Op($id, $op) { Call POST "/api/instances/$id/$op" }
function View($id) { Call GET "/api/instances/$id" }

function Tasks($node) {
    if ($node.type -eq 'task') { return ,$node }
    $all = @()
    foreach ($c in $node.children) { $all += Tasks $c }
    return $all
}
function StateOf($view, $task) { ((Tasks $view.tree) | Where-Object { $_.id -eq $task }).state }

# assign -> start -> complete, the normal manual path
function Do-Manual($id, $task, $who, $outcome = '') {
    $null = Act $id $task 'assign' @{ participant = $who }
    $null = Act $id $task 'start'
    return Act $id $task 'complete' @{ outcome = $outcome }
}
# assign -> start -> execute, for automated and approval tasks
function Do-Execute($id, $task, $who) {
    $null = Act $id $task 'assign' @{ participant = $who }
    $null = Act $id $task 'start'
    return Act $id $task 'execute'
}

Write-Host "Reset"
$r = Call POST '/api/reset'
Check ($r.ok) 'reset rebuilds both scenarios'
$meta = Call GET '/api/meta'
Check ($meta.industries.Count -eq 2) 'meta lists two industries'

# ------------------------------------------------------------------ EDUCATION
Write-Host "`nEducation - completion path (Thandiwe, faculty reviewer approves)"
$a = New-Instance 'education' 'thandiwe'
$b = New-Instance 'education' 'thandiwe'
Check ($a -eq 'EDU-1' -and $b -eq 'EDU-2') 'two independent instances created'
$s = Op $a 'start'
Check ($s.ok -and (StateOf $s.instance 'SubmitApplication') -eq 'Available') 'start makes Submit Application available'
$bad = Act $a 'CheckDocuments' 'complete'
Check (-not $bad.ok -and $bad.message -like '*Created*') 'complete() on a Created task is refused by the state'
$bad = Act $a 'CheckDocuments' 'makeAvailable'
Check (-not $bad.ok -and $bad.message -like '*Dependency*') 'makeAvailable() refused by the Mediator before the prerequisite completes'
$r = Do-Manual $a 'SubmitApplication' 'applicant'
Check ((StateOf $r.instance 'CheckDocuments') -eq 'Available') 'completing Submit Application unlocks Check Documents'
$r = Do-Manual $a 'CheckDocuments' 'khumalo'
Check ((StateOf $r.instance 'VerifyPersonalDetails') -eq 'Available' -and (StateOf $r.instance 'VerifyResults') -eq 'Available') 'fork: both verifications unlock'
$r = Do-Execute $a 'VerifyPersonalDetails' 'system'
Check ((StateOf $r.instance 'VerifyPersonalDetails') -eq 'Completed') 'automated ID check completes through the adapter'
Check ((StateOf $r.instance 'FacultyChecksRequirements') -eq 'Created') 'join: faculty check still waits for the second branch'
$r = Do-Execute $a 'VerifyResults' 'system'
Check ((StateOf $r.instance 'FacultyChecksRequirements') -eq 'Available') 'join: faculty check unlocks after both branches'
$r = Do-Manual $a 'FacultyChecksRequirements' 'botha'
$r = Do-Execute $a 'ApplicationDecision' 'botha'
Check ((StateOf $r.instance 'ApplicationDecision') -eq 'Completed' -and $r.message -like '*Faculty reviewer approved*') 'risk 15: the chain stops at the faculty reviewer'
$r = Do-Execute $a 'NotifyApplicant' 'system'
Check ($r.instance.status -eq 'Finished' -and $r.instance.outcome -like 'Admitted*') 'applicant notified; instance Finished'
$v = View $b
Check ((StateOf $v 'SubmitApplication') -eq 'Created' -and $v.status -eq 'Created') 'isolation: EDU-2 untouched by all of that'

Write-Host "`nEducation - undo and snapshot (EDU-2)"
$null = Op $b 'start'
$r = Act $b 'SubmitApplication' 'assign' @{ participant = 'applicant' }
Check ((StateOf $r.instance 'SubmitApplication') -eq 'Assigned' -and $r.instance.canUndo) 'assign pushed a command'
$r = Op $b 'undo'
Check ($r.ok -and (StateOf $r.instance 'SubmitApplication') -eq 'Available') 'undo restores Available'
$r = Op $b 'undo'
Check (-not $r.ok) 'undo with an empty history is refused politely'
$null = Op $b 'snapshot'
$null = Do-Manual $b 'SubmitApplication' 'applicant'
$r = Op $b 'restore'
Check ($r.ok -and (StateOf $r.instance 'SubmitApplication') -eq 'Available' -and (StateOf $r.instance 'CheckDocuments') -eq 'Created') 'restore snapshot rolls the tree back'
$r = Do-Manual $b 'SubmitApplication' 'applicant'
Check ((StateOf $r.instance 'CheckDocuments') -eq 'Available') 'dependencies still work on the restored tree'

Write-Host "`nEducation - exception paths"
$k = New-Instance 'education' 'kagiso'
$null = Op $k 'start'
$null = Do-Manual $k 'SubmitApplication' 'applicant'
$null = Act $k 'CheckDocuments' 'assign' @{ participant = 'khumalo' }
$null = Act $k 'CheckDocuments' 'start'
$r = Act $k 'CheckDocuments' 'reject'
Check ((StateOf $r.instance 'CheckDocuments') -eq 'Rejected' -and (StateOf $r.instance 'VerifyResults') -eq 'Created') 'documents missing: Check Documents rejected, verification stays locked'
$r = Do-Manual $k 'CheckDocuments' 'steyn'
Check ((StateOf $r.instance 'VerifyResults') -eq 'Available') 'loop back: reassigned after documents arrive, then fork'

$sp = New-Instance 'education' 'sipho'
$null = Op $sp 'start'
foreach ($t in @(@('SubmitApplication','applicant'), @('CheckDocuments','khumalo'))) { $null = Do-Manual $sp $t[0] $t[1] }
$null = Do-Execute $sp 'VerifyPersonalDetails' 'system'
$null = Do-Execute $sp 'VerifyResults' 'system'
$null = Do-Manual $sp 'FacultyChecksRequirements' 'botha'
$r = Do-Execute $sp 'ApplicationDecision' 'botha'
Check ((StateOf $r.instance 'ApplicationDecision') -eq 'Rejected' -and $r.message -like '*Registrar rejected*') 'risk 91: chain escalates to the registrar, who rejects'
Check ((StateOf $r.instance 'NotifyApplicant') -eq 'Available') 'rejection still routes to Notify Applicant'

$ai = New-Instance 'education' 'aisha'
$null = Op $ai 'start'
foreach ($t in @(@('SubmitApplication','applicant'), @('CheckDocuments','khumalo'))) { $null = Do-Manual $ai $t[0] $t[1] }
$r = Do-Execute $ai 'VerifyResults' 'system'
Check ((StateOf $r.instance 'VerifyResults') -eq 'InProgress') 'Cambridge results not in NSC database: connector fails, task stays InProgress'
$r = Act $ai 'VerifyResults' 'escalate'
Check ((StateOf $r.instance 'VerifyResults') -eq 'Escalated') 'escalated'
$r = Do-Manual $ai 'VerifyResults' 'khumalo'
Check ((StateOf $r.instance 'VerifyResults') -eq 'Completed') 'reassigned to an admissions officer and completed by hand'

$p = New-Instance 'education' 'pieter'
$null = Op $p 'start'
foreach ($t in @(@('SubmitApplication','applicant'), @('CheckDocuments','khumalo'))) { $null = Do-Manual $p $t[0] $t[1] }
$null = Do-Execute $p 'VerifyPersonalDetails' 'system'
$null = Do-Execute $p 'VerifyResults' 'system'
$null = Do-Manual $p 'FacultyChecksRequirements' 'botha'
$r = Do-Execute $p 'ApplicationDecision' 'botha'
Check ($r.message -like '*Head of Department approved*') 'risk 48: borderline case escalates to the HOD'

# ------------------------------------------------------------------ HEALTHCARE
Write-Host "`nHealthcare - routine completion path with slot contention (Johan)"
$j = New-Instance 'healthcare' 'johan'
$null = Op $j 'start'
$null = Do-Manual $j 'RegisterPatient' 'mthembu'
$r = Do-Execute $j 'VerifyMedicalAid' 'system'
Check ((StateOf $r.instance 'VerifyMedicalAid') -eq 'Completed') 'medical aid pre-authorised through the payment adapter'
$bad = Act $j 'BookAppointment' 'makeAvailable'
Check (-not $bad.ok) 'no booking before triage (Mediator refuses)'
$r = Do-Manual $j 'TriageAssessment' 'mahlangu' 'routine'
Check ((StateOf $r.instance 'BookAppointment') -eq 'Available' -and (StateOf $r.instance 'EmergencyConsultation') -eq 'Cancelled') 'routing: routine -> booking branch, emergency branch cancelled'
$r = Act $j 'BookAppointment' 'assign' @{ participant = 'naidoo' }
Check ($r.ok -and $r.message -like '*refused a slot*') 'Dr Naidoo has no free slot: ResourceCoordinator refuses'
$r = Act $j 'BookAppointment' 'start'
Check (-not $r.ok) 'booking cannot start without a granted slot'
$r = Act $j 'BookAppointment' 'escalate'
Check ((StateOf $r.instance 'BookAppointment') -eq 'Escalated') 'escalated'
$r = Act $j 'BookAppointment' 'assign' @{ participant = 'mokoena' }
Check ($r.message -like '*Slot granted*') 'reassigned to Dr Mokoena: slot granted'
$res = Call GET '/api/resources/healthcare'
Check ((($res | Where-Object { $_.id -eq 'mokoena' }).grantedTo | Measure-Object).Count -eq 1) 'resources view shows the engine grant'
$null = Act $j 'BookAppointment' 'start'
$r = Act $j 'BookAppointment' 'complete'
Check ((StateOf $r.instance 'SendReminder') -eq 'Available' -and (StateOf $r.instance 'GPConsultation') -eq 'Available') 'booking unlocks reminder and consultation'
$r = Do-Execute $j 'SendReminder' 'system'
Check ((StateOf $r.instance 'SendReminder') -eq 'Completed') 'SMS reminder sent through the messaging adapter'
$null = Do-Manual $j 'GPConsultation' 'mokoena'
$r = Do-Execute $j 'SpecialistReferral' 'mahlangu'
Check ($r.message -like '*Nurse approved*' -and $r.instance.status -eq 'Finished') 'R450 physio referral: the nurse approves; visit Finished'

Write-Host "`nHealthcare - urgent routing (Lerato)"
$l = New-Instance 'healthcare' 'lerato'
$null = Op $l 'start'
$null = Do-Manual $l 'RegisterPatient' 'mthembu'
$null = Do-Execute $l 'VerifyMedicalAid' 'system'
$r = Do-Manual $l 'TriageAssessment' 'dube' 'urgent'
Check ((StateOf $r.instance 'EmergencyConsultation') -eq 'Available' -and (StateOf $r.instance 'BookAppointment') -eq 'Cancelled') 'urgent -> emergency branch, booking branch cancelled'
$r = Do-Manual $l 'EmergencyConsultation' 'sibiya'
Check ($r.instance.status -eq 'Finished') 'urgent visit Finished'

Write-Host "`nHealthcare - exception paths"
$f = New-Instance 'healthcare' 'fatima'
$null = Op $f 'start'
$null = Do-Manual $f 'RegisterPatient' 'mthembu'
$r = Do-Execute $f 'VerifyMedicalAid' 'system'
Check ((StateOf $r.instance 'VerifyMedicalAid') -eq 'InProgress' -and $r.message -like '*declined*') 'suspended medical aid: adapter declines'
$null = Act $f 'VerifyMedicalAid' 'escalate'
$r = Do-Manual $f 'VerifyMedicalAid' 'mthembu'
Check ((StateOf $r.instance 'TriageAssessment') -eq 'Available') 'reception completes it as a cash patient; triage unlocks'
$null = Do-Manual $f 'TriageAssessment' 'mahlangu' 'routine'
$null = Act $f 'BookAppointment' 'assign' @{ participant = 'vdmerwe' }
$null = Act $f 'BookAppointment' 'start'
$null = Act $f 'BookAppointment' 'complete'
$null = Do-Manual $f 'GPConsultation' 'vdmerwe'
$r = Do-Execute $f 'SpecialistReferral' 'mahlangu'
Check ((StateOf $r.instance 'SpecialistReferral') -eq 'Rejected' -and $r.message -like '*Clinical manager*rejected*') 'R31 500 referral: travels the whole chain and is rejected'

$m = New-Instance 'healthcare' 'mandla'
$null = Op $m 'start'
$null = Do-Manual $m 'RegisterPatient' 'mthembu'
$null = Do-Execute $m 'VerifyMedicalAid' 'system'
$null = Do-Manual $m 'TriageAssessment' 'mahlangu' 'routine'
$null = Act $m 'BookAppointment' 'assign' @{ participant = 'vdmerwe' }
$null = Act $m 'BookAppointment' 'start'
$null = Act $m 'BookAppointment' 'complete'
$r = Act $m 'GPConsultation' 'cancel'
Check ((StateOf $r.instance 'GPConsultation') -eq 'Cancelled' -and (StateOf $r.instance 'SpecialistReferral') -eq 'Cancelled' -and $r.instance.outcome -like '*no-show*') 'no-show: consultation cancelled, follow-ups cancelled, slot released'
$r = Op $m 'undo'
Check ((StateOf $r.instance 'GPConsultation') -eq 'Available' -and $r.instance.data.'resource.BookAppointment') 'undo brings the consultation and the slot back'

$bo = New-Instance 'healthcare' 'bongani'
$null = Op $bo 'start'
$null = Do-Manual $bo 'RegisterPatient' 'mthembu'
$null = Do-Execute $bo 'VerifyMedicalAid' 'system'
$null = Do-Manual $bo 'TriageAssessment' 'mahlangu' 'routine'
$null = Act $bo 'BookAppointment' 'assign' @{ participant = 'mokoena' }
$null = Act $bo 'BookAppointment' 'start'
$null = Act $bo 'BookAppointment' 'complete'
$null = Do-Manual $bo 'GPConsultation' 'mokoena'
$r = Do-Execute $bo 'SpecialistReferral' 'mahlangu'
Check ($r.message -like '*GP -> Specialist; Specialist approved*') 'R9 800 MRI: Nurse -> GP -> Specialist approves'

Write-Host "`nErrors"
$r = Call GET '/api/instances/NOPE-1'
Check (-not $r.ok -and $r.message -like '*No instance*') 'unknown instance -> friendly 404'
$r = Act $j 'NoSuchTask' 'start'
Check (-not $r.ok) 'unknown task -> friendly 404'
$r = Act $j 'GPConsultation' 'dance'
Check (-not $r.ok -and $r.message -like '*Unknown action*') 'unknown action -> 400'
$list = Call GET '/api/instances'
Check ($list.Count -eq 11) 'instance list has all 11 instances'

Write-Host "`n$($script:passed) passed, $($script:failed) failed"
exit $script:failed
