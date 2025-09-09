param(
  [string]$Py = "",   # Path to python.exe (optional) for mpremote
  [string]$UF2 = ""   # Path to the UF2 to flash (optional). If empty, we auto-pick.
)

$ErrorActionPreference = "Stop"

# Workspace root (this script is in /tools)
$wsRoot = Split-Path -Path $PSScriptRoot -Parent

# Pick a UF2 if not provided: prefer files starting with "micro", else any *.uf2 in repo root.
if (-not $UF2 -or -not (Test-Path -LiteralPath $UF2)) {
  $uf2Cand = Get-ChildItem -Path $wsRoot -Filter micro*.uf2 -File -ErrorAction SilentlyContinue |
             Sort-Object LastWriteTime -Descending | Select-Object -First 1
  if (-not $uf2Cand) {
    $uf2Cand = Get-ChildItem -Path $wsRoot -Filter *.uf2 -File -ErrorAction SilentlyContinue |
               Sort-Object LastWriteTime -Descending | Select-Object -First 1
  }
  if (-not $uf2Cand) {
    Write-Error "No UF2 found in: $wsRoot. Put your MicroPython UF2 in the repo root or pass -UF2 <path>."
  }
  $UF2 = $uf2Cand.FullName
}

Write-Host ("Using UF2: " + $UF2)

# Try to enter BOOTSEL via mpremote (works when board is already running MicroPython)
if ($Py -and (Test-Path -LiteralPath $Py)) {
  Write-Host "Attempting to enter BOOTSEL via mpremote..."
  try {
    & $Py -m mpremote exec "import machine; machine.bootloader()" | Out-Null
    Start-Sleep -Milliseconds 800
  } catch {
    Write-Host "mpremote bootloader() call failed (device may not be in MicroPython yet). Continuing..."
  }
}

# Wait up to 30s for RPI-RP2 mass-storage drive
$drive = $null
for ($i = 1; $i -le 60; $i++) {
  try {
    $drive = Get-CimInstance -ClassName Win32_LogicalDisk -ErrorAction SilentlyContinue |
             Where-Object { $_.VolumeName -eq "RPI-RP2" } | Select-Object -First 1
  } catch {
    $drive = $null
  }
  if ($drive) { break }
  Start-Sleep -Milliseconds 500
}

if (-not $drive) {
  Write-Error "RPI-RP2 drive not found. Hold BOOTSEL and plug in the Pico, then re-run this task."
}

$dest = ($drive.DeviceID + "\")
Write-Host ("Flashing to: " + $dest)
Copy-Item -LiteralPath $UF2 -Destination $dest -Force

Write-Host "Firmware flashed successfully. The Pico will reboot into MicroPython."