param(
  [string]$Py = "",              # Optional: path to python.exe; if empty will use "python" on PATH
  [string]$DeviceDir = "Pico_w\\device", # Source dir for .py files
  [string]$soundsDir = "Pico_w\\sounds"  # Source dir for .wav files
)

$ErrorActionPreference = "Stop"

# Resolve workspace root
$ws = Split-Path $PSScriptRoot -Parent
Set-Location $ws

function PyRun {
  param([string[]]$mpArgs)
  $python = if ($Py -and (Test-Path $Py)) { $Py } else { "python" }
  & $python -m mpremote @mpArgs
}

function Get-FirstPort {
  try {
    $out = PyRun @("devs") 2>$null
    if (-not $out) { return $null }
    foreach ($line in $out) {
      if ($line -match "COM\d+") {
        return ($matches[0])
      }
    }
  } catch {
    return $null
  }
  return $null
}

function Wait-For-Device {
  param([int]$TimeoutMs = 20000)
  $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
  while ([DateTime]::UtcNow -lt $deadline) {
    $port = Get-FirstPort
    if ($port) { return $true }
    Start-Sleep -Milliseconds 500
  }
  return $false
}

Write-Host "Checking for MicroPython device..."

if (-not (Wait-For-Device)) {
  $hint = @(
    "MicroPython device not found after waiting.",
    "Tips:",
    "  - Make sure the board is NOT in BOOTSEL (RPI-RP2 drive should NOT be visible).",
    "  - Close Thonny or any serial monitor so the port is free.",
    "  - Unplug/replug USB, wait 2-3 seconds.",
    "  - Run 'Host: List COM ports' to confirm Windows sees it."
  )
  throw ($hint -join "`n")
}

# Copy Python files
$files = @("boot.py","config.py","audio.py","main.py")
foreach ($f in $files) {
  $src = Join-Path $ws (Join-Path $DeviceDir $f)
  if (Test-Path $src) {
    Write-Host ("  cp " + $src + " -> :" + $f)
    PyRun @("connect","auto","cp",$src,":$f")
  } else {
    Write-Host ("  (skip missing file: " + $src + ")")
  }
}

# Copy WAV sounds (if present)
$assDir = Join-Path $ws $soundsDir
if (Test-Path $assDir) {
  $wavs = Get-ChildItem -Path $assDir -Filter *.wav -File -ErrorAction SilentlyContinue
  if ($wavs.Count -gt 0) {
    # Try to ensure /sounds exists on device (ignore errors if it already exists)
    try { PyRun @("connect","auto","mkdir",":sounds") } catch { }

    foreach ($w in $wavs) {
      $dst = ":sounds/" + $w.Name
      Write-Host ("  cp " + $w.FullName + " -> " + $dst)
      PyRun @("connect","auto","cp",$w.FullName,$dst)
    }
  } else {
    Write-Host "  (no .wav files found in sounds/; skipping WAV copy)"
  }
} else {
  Write-Host "  (no sounds folder; skipping WAV copy)"
}

Write-Host "Resetting device..."
PyRun @("connect","auto","reset")