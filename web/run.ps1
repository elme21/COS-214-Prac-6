# Starts the TIVIDY web demo at http://localhost:8080 (builds first if needed).
# Usage (from any folder):   .\web\run.ps1  [-Port 8080] [-Rebuild] [-NoBrowser]
# Stop with Ctrl+C. Restarting resets the demo: all state lives in the server's memory.
param([int]$Port = 8080, [switch]$Rebuild, [switch]$NoBrowser)

$Web = $PSScriptRoot
$Exe = Join-Path $Web 'build\tividy-web.exe'

if ($Rebuild -or -not (Test-Path $Exe)) {
    & (Join-Path $Web 'build.ps1')
    if (-not (Test-Path $Exe)) { Write-Error 'Build failed; see the messages above.'; exit 1 }
}

if (-not $NoBrowser) {
    # Opens the page shortly after the server starts listening.
    Start-Job -ScriptBlock { param($u) Start-Sleep -Seconds 1; Start-Process $u } -ArgumentList "http://localhost:$Port/" | Out-Null
}
# Every web/templates/*.json is loaded as a scenario at startup.
& $Exe --public (Join-Path $Web 'public') --templates (Join-Path $Web 'templates') --port $Port
