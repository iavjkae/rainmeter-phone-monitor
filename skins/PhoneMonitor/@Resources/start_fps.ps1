# SPDX-License-Identifier: MIT
$ErrorActionPreference = 'Stop'
$fpsBridgePath = Join-Path $PSScriptRoot 'FPSBridge.exe'
$presentMonPath = Join-Path $PSScriptRoot 'PresentMon.exe'
if (-not (Test-Path -LiteralPath $presentMonPath)) {
    throw 'PresentMon.exe is missing. Run tools/fetch_presentmon.py and tools/package.py first.'
}
Start-Process -FilePath $fpsBridgePath -WorkingDirectory $PSScriptRoot -Verb RunAs -WindowStyle Hidden
