# Builds the TIVIDY web demo: the real engine (../src, ../include) + web/server + web/scenarios.
# Usage (from any folder):   .\web\build.ps1 [-Clean]
# All output goes to web\build\. No file outside web\ is written.
# Every path is derived from this script's own folder ($PSScriptRoot), so the build works
# wherever the repository is cloned. It needs only include/, src/ and web/ - no stubs.
param([switch]$Clean)

# 'Continue' so compiler warnings on stderr never abort the build; failures are caught via $LASTEXITCODE.
$ErrorActionPreference = 'Continue'
$Web     = $PSScriptRoot
$Project = Split-Path -Parent $Web              # repository root: holds include/ and src/
$Build   = Join-Path $Web 'build'
$Obj     = Join-Path $Build 'obj'
$Exe     = Join-Path $Build 'tividy-web.exe'

if ($Clean -and (Test-Path $Build)) { Remove-Item -Recurse -Force $Build }
New-Item -ItemType Directory -Force $Obj | Out-Null

# Engine files left out of the build, and why (see web/INTEGRATION.md). Every engine file builds
# today, so the list is empty; add a line here if a teammate's file ever stops compiling.
$ExcludedEngineFiles = [ordered]@{}

# Per-file extra flags for engine files (none needed today).
$EngineFileFlags = @{}

$engine    = Get-ChildItem (Join-Path $Project 'src') -Filter *.cpp | Where-Object { -not $ExcludedEngineFiles.Contains($_.Name) }
$scenarios = (Get-ChildItem (Join-Path $Web 'scenarios') -Filter *.cpp).FullName
$server    = (Get-ChildItem (Join-Path $Web 'server') -Filter *.cpp).FullName

$common = @('-std=c++11', '-Wall', '-Wextra', '-O1')
$includes = @(
    '-I', (Join-Path $Project 'include'),
    '-I', (Join-Path $Web 'server'),
    '-I', (Join-Path $Web 'scenarios'),
    '-I', (Join-Path $Web 'third_party\httplib'),
    '-I', (Join-Path $Web 'third_party\nlohmann')
)

# Newest header anywhere we include from: if it changed, everything is rebuilt.
$headerDirs = @((Join-Path $Project 'include'), (Join-Path $Web 'server'), (Join-Path $Web 'scenarios'))
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
foreach ($k in $ExcludedEngineFiles.Keys) { Write-Host "  skipped src/$k - $($ExcludedEngineFiles[$k])" }
$objects = @()
foreach ($f in $engine) {
    $extra = @()
    if ($EngineFileFlags.ContainsKey($f.Name)) { $extra = $EngineFileFlags[$f.Name] }
    $objects += Compile $f.FullName 'engine' $extra
}
foreach ($f in $scenarios) { $objects += Compile $f 'scenarios' @() }
foreach ($f in $server)    { $objects += Compile $f 'server' @() }

Write-Host '  linking build\tividy-web.exe'
& g++ -std=c++11 @objects -o $Exe -static -lws2_32
if ($LASTEXITCODE -ne 0) { throw 'Link failed' }
Write-Host "Done: $Exe"
