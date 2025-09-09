$ErrorActionPreference = "Stop"

# Workspace root is the parent of .vscode
$wsRoot = Split-Path $PSScriptRoot -Parent
$build  = Join-Path $wsRoot "build_ninja"

Write-Host "Workspace: $wsRoot"
Write-Host "Build dir: $build"

# Ensure build folder exists
if (-not (Test-Path $build)) {
    Write-Error "Build folder not found: $build. Run a build first (e.g., 'Build (manual)' or 'Build+Flash')."
    exit 1
}

# Find the newest UF2 (search within build folder)
$uf2 = Get-ChildItem -Path $build -Recurse -Filter *.uf2 -File |
       Sort-Object LastWriteTime -Descending |
       Select-Object -First 1

if (-not $uf2) {
    Write-Error "No UF2 found in: $build. Make sure your CMake target produces a .uf2 (e.g., 'kws_features.uf2')."
    exit 1
}

Write-Host ("UF2 found: " + $uf2.FullName)

# Wait briefly for the Pico BOOTSEL drive (RPI-RP2) to appear
$rp = $null
for ($i=1; $i -le 10; $i++) {
    $rp = Get-CimInstance Win32_LogicalDisk | Where-Object { $_.VolumeName -eq "RPI-RP2" } | Select-Object -First 1
    if ($rp) { break }
    Start-Sleep -Milliseconds 500
}

if (-not $rp) {
    Write-Error "RPI-RP2 not mounted. Put the Pico in BOOTSEL mode (hold BOOTSEL, plug in)."
    exit 1
}

Write-Host ("Flashing to drive: " + $rp.DeviceID)
Copy-Item -Path $uf2.FullName -Destination ($rp.DeviceID + "\") -Force
Write-Host ("✅ Flashed " + $uf2.Name + " to " + $rp.DeviceID)