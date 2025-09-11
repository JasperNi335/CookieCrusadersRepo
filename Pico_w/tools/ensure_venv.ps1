param(
  [Parameter(Mandatory = $true)]
  [string]$Workspace
)

$py = Join-Path $Workspace "Pico_w\.venv\Scripts\python.exe"

Write-Host "Workspace: $Workspace"
if (Test-Path -LiteralPath $py) {
  Write-Host "venv OK ($py)"
  exit 0
}

Write-Host "No venv found -> bootstrapping..."
$bootstrap = Join-Path $Workspace "Pico_w\tools\bootstrap.py"

# Use the user's Python on PATH to run the bootstrap
& python $bootstrap
if ($LASTEXITCODE -ne 0) {
  throw "Bootstrap failed (exit $LASTEXITCODE)"
}

Write-Host "Bootstrap done."
exit 0