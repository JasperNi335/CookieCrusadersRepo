param(
  [string]$UF2 = "",
  [string]$Py = "python"
)

$ErrorActionPreference = "Stop"

function Find-UF2 {
  param([string]$Root)
  if ($UF2 -ne "" -and (Test-Path $UF2)) { return (Resolve-Path $UF2).Path }
  $candidates = Get-ChildItem -Path $Root -Filter *.uf2 -File | Sort-Object LastWriteTime -Descending
  if ($candidates.Count -eq 0) {
    throw "No .uf2 found in $Root. Put a MicroPython UF2 in the repo root or pass -UF2 <path>."
  }
  return $candidates[0].FullName
}

$ws = Split-Path $PSScriptRoot -Parent
$uf2Path = Find-UF2 -Root $ws
Write-Host "Using UF2: $uf2Path"

# Try to enter BOOTSEL (ROM) via MicroPython if the board is currently running it.
try {
  & $Py -m mpremote exec "import machine; machine.bootloader()" | Out-Null
  Start-Sleep -Milliseconds 500
} catch {
  Write-Host "mpremote not available or board not in MicroPython; will look for BOOTSEL drive..."
}

# Wait for the UF2 drive to appear
$drive = $null
for ($i=0; $i -lt 40; $i++) {
  $drive = Get-CimInstance Win32_LogicalDisk |
           Where-Object { $_.VolumeName -eq "RPI-RP2" } |
           Select-Object -First 1
  if ($drive) { break }
  Start-Sleep -Milliseconds 250
}
if (-not $drive) {
  throw "BOOTSEL drive 'RPI-RP2' not found. Hold BOOTSEL while plugging USB, then re-run."
}

Write-Host "Flashing to $($drive.DeviceID)..."
Copy-Item -Path $uf2Path -Destination "$($drive.DeviceID)\" -Force

# Wait for drive to disappear (device reboots)
for ($i=0; $i -lt 40; $i++) {
  $still = Get-CimInstance Win32_LogicalDisk |
           Where-Object { $_.VolumeName -eq "RPI-RP2" } |
           Select-Object -First 1
  if (-not $still) { break }
  Start-Sleep -Milliseconds 250
}

Write-Host "Firmware flashed. The Pico is rebooting into MicroPython."