# Builds the TIVIDY web demo: the real engine (../src, ../include) + web/server + web/scenarios.
# Usage (from any folder):   .\web\build.ps1 [-Clean]
# All output goes to web\build\. No file outside web\ is written.
# Every path is derived from this script's own folder ($PSScriptRoot), so the build works
# wherever the repository is cloned. It needs only include/, src/ and web/.
param([switch]$Clean)

# 'Continue' so compiler warnings on stderr never abort the build; failures are caught via $LASTEXITCODE.
$ErrorActionPreference = 'Continue'
$Web     = $PSScriptRoot
$Project = Split-Path -Parent $Web              # repository root: holds include/ and src/
$StubDir = Join-Path $Web 'temporary_stubs'      # TEMPORARY Connector stubs (see INTEGRATION.md, section 13)
$Build   = Join-Path $Web 'build'
$Obj     = Join-Path $Build 'obj'
$Exe     = Join-Path $Build 'tividy-web.exe'

if ($Clean -and (Test-Path $Build)) { Remove-Item -Recurse -Force $Build }
New-Item -ItemType Directory -Force $Obj | Out-Null

# Engine files that do not compile yet (see web/INTEGRATION.md). Remove a name here once it is fixed.
$ExcludedEngineFiles = @(
    'ApproveCommand.cpp', 'AssignCommand.cpp', 'CancelCommand.cpp', 'RejectCommand.cpp'
)

$engine    = (Get-ChildItem (Join-Path $Project 'src') -Filter *.cpp | Where-Object { $ExcludedEngineFiles -notcontains $_.Name }).FullName
# TEMPORARY: stub Connector / ConnectorManager until Member 7's real classes exist.
$stubs     = (Get-ChildItem $StubDir -Filter *.cpp).FullName
$scenarios = (Get-ChildItem (Join-Path $Web 'scenarios') -Filter *.cpp).FullName
$server    = (Get-ChildItem (Join-Path $Web 'server') -Filter *.cpp).FullName

$common = @('-std=c++11', '-Wall', '-Wextra', '-O1')
# Order matters: the real include/ wins; temporary_stubs only supplies Connector.h / ConnectorManager.h.
$includes = @(
    '-I', (Join-Path $Project 'include'),
    '-I', (Join-Path $Web 'server'),
    '-I', (Join-Path $Web 'scenarios'),
    '-I', (Join-Path $Web 'third_party\httplib'),
    '-I', (Join-Path $Web 'third_party\nlohmann'),
    '-I', $StubDir,
    '-I', $Project                      # src/WorkflowManager.cpp includes "include/WorkflowManager.h"
)
# src/WorkStage.cpp uses size_t without <cstddef>; force-include it for engine files only.
$engineFlags = @('-include', 'cstddef')

# Newest header anywhere we include from: if it changed, everything is rebuilt.
$headerDirs = @((Join-Path $Project 'include'), $StubDir, (Join-Path $Web 'server'), (Join-Path $Web 'scenarios'))
$newestHeader = (Get-ChildItem $headerDirs -Include *.h, *.hpp -Recurse | Measure-Object -Property LastWriteTime -Maximum).Maximum

function Compile($source, $group, $extra) {
    $name = [IO.Path]::GetFileNameWithoutExtension($source)
    $object = Join-Path $Obj ($group + '_' + $name + '.o')
    if (Test-Path $object) {
        $objTime = (Get-Item $object).LastWriteTime
        if ($objTime -gt (Get-Item $source).LastWriteTime -and $objTime -gt $newestHeader) { return $object }
    }
    Write-Host "  compiling $group/$name.cpp"
    & g++ @common @extra @includes -c $source -o $object
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $source" }
    return $object
}

Write-Host 'Building TIVIDY web demo (C++11)...'
$objects = @()
foreach ($f in $engine)    { $objects += Compile $f 'engine' $engineFlags }
foreach ($f in $stubs)     { $objects += Compile $f 'stub' @() }
foreach ($f in $scenarios) { $objects += Compile $f 'scenarios' @() }
foreach ($f in $server)    { $objects += Compile $f 'server' @() }

Write-Host '  linking build\tividy-web.exe'
& g++ -std=c++11 @objects -o $Exe -static -lws2_32
if ($LASTEXITCODE -ne 0) { throw 'Link failed' }
Write-Host "Done: $Exe"
