# .vscode/flash.ps1
[CmdletBinding()]
param(
  [string]$BuildDir = "",                # optional override; if blank we'll auto-detect
  [string]$VolumeLabel = "RPI-RP2"       # Pico BOOTSEL volume label
)

$ErrorActionPreference = "Stop"

# Workspace root is the parent of .vscode/
$wsRoot = Split-Path $PSScriptRoot -Parent

# Resolve build directory
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
  $candidates = @(
    (Join-Path $wsRoot "build"),
    (Join-Path $wsRoot "build_ninja")
  )
  $BuildDir = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
  if (-not $BuildDir) {
    Write-Error "No build folder found. Tried: $($candidates -join ', '). Run a build first."
    exit 1
  }
}

Write-Host "Workspace: $wsRoot"
Write-Host "Build dir: $BuildDir"

# Find newest UF2
$uf2 = Get-ChildItem -Path $BuildDir -Recurse -Filter *.uf2 -File |
       Sort-Object LastWriteTime -Descending |
       Select-Object -First 1
if (-not $uf2) {
  Write-Error "No .uf2 found under: $BuildDir. Build the target (e.g., 'kws_features.uf2') first."
  exit 1
}
Write-Host "UF2: $($uf2.FullName)"

# Find BOOTSEL drive (prefer Get-Volume; fall back to WMI)
$vol = $null
try {
  $vol = Get-Volume | Where-Object { $_.FileSystemLabel -eq $VolumeLabel } | Select-Object -First 1
} catch { }
if (-not $vol) {
  $rp = Get-CimInstance Win32_LogicalDisk | Where-Object { $_.VolumeName -eq $VolumeLabel } | Select-Object -First 1
  if (-not $rp) {
    Write-Error "RPI-RP2 not mounted. Hold BOOTSEL and plug in the Pico, then try again."
    exit 1
  }
  $destRoot = $rp.DeviceID + "\"
} else {
  $destRoot = ($vol.DriveLetter + ":\")
}

$dest = Join-Path $destRoot $uf2.Name
Write-Host "Copying to $dest ..."
Copy-Item -Path $uf2.FullName -Destination $dest -Force
Write-Host "✅ Flashed $($uf2.Name) to $destRoot"