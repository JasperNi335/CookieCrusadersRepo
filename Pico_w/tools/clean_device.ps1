param(
  [string]$Py = "",
  [string]$Port = ""
)

$ErrorActionPreference = "Stop"
$wsRoot = Split-Path -Path $PSScriptRoot -Parent

if (-not $Py -or -not (Test-Path -LiteralPath $Py)) {
  $Py = Join-Path $wsRoot "Pico_w\.venv\Scripts\python.exe"
  if (-not (Test-Path -LiteralPath $Py)) {
    Write-Error "Python not found. Pass -Py <path to python.exe> or create .venv."
  }
}

$base = @("-m", "mpremote")
if ($Port) { $base += @("connect", $Port) }

$files = @(":boot.py", ":config.py", ":audio.py", ":main.py")
foreach ($f in $files) {
  Write-Host ("rm " + $f)
  & $Py @base "rm" $f | Out-Null
}
Write-Host "Resetting device..."
& $Py @base "reset"