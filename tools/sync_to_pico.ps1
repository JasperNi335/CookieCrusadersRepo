param(
  [string]$Py = "",     # path to python.exe (optional; defaults to .venv)
  [string]$Port = ""    # e.g. COM6 (optional; lets mpremote auto-pick if empty)
)

$ErrorActionPreference = "Stop"

# Workspace root (script lives in /tools)
$wsRoot = Split-Path -Path $PSScriptRoot -Parent

# Resolve python path (default to .venv)
if (-not $Py -or -not (Test-Path -LiteralPath $Py)) {
  $Py = Join-Path $wsRoot ".venv\Scripts\python.exe"
  if (-not (Test-Path -LiteralPath $Py)) {
    Write-Error "Python not found. Pass -Py <path to python.exe> or create .venv."
  }
}

# Files to push (absolute paths)
$files = @(
  (Join-Path $wsRoot "device\boot.py"),
  (Join-Path $wsRoot "device\config.py"),
  (Join-Path $wsRoot "device\audio.py"),
  (Join-Path $wsRoot "device\main.py")
)

foreach ($f in $files) {
  if (-not (Test-Path -LiteralPath $f)) {
    Write-Error "Missing file: $f"
  }
}

# Build base arg list for mpremote
$base = @("-m", "mpremote")
if ($Port) { $base += @("connect", $Port) }

Write-Host "Syncing files to Pico..."
foreach ($f in $files) {
  $dest = ":" + [System.IO.Path]::GetFileName($f)
  Write-Host ("  cp {0} -> {1}" -f $f, $dest)
  & $Py @base "cp" $f $dest
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Write-Host "Resetting device..."
& $Py @base "reset"
exit $LASTEXITCODE