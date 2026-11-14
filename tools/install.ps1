# SPDX-License-Identifier: MIT
param(
    [Parameter(Mandatory = $true)][string]$SkinPath,
    [Parameter(Mandatory = $true)][string]$PluginPath,
    [string]$PackagePath = (Join-Path $PSScriptRoot '..\dist')
)
$ErrorActionPreference = 'Stop'
$package = (Resolve-Path -LiteralPath $PackagePath).Path
$source = Join-Path $package 'Skins\PhoneMonitor'
$plugin = Join-Path $package 'Plugins\PhoneTelemetry.dll'
if (-not (Test-Path -LiteralPath $source) -or -not (Test-Path -LiteralPath $plugin)) {
    throw 'Package is incomplete. Build and run tools/package.py first.'
}
if (Get-Process -Name Rainmeter -ErrorAction SilentlyContinue) {
    throw 'Exit Rainmeter before installation so the plugin DLL is not locked.'
}
$null = New-Item -ItemType Directory -Path $SkinPath, $PluginPath -Force
$destination = Join-Path $SkinPath 'PhoneMonitor'
$savedSettings = @{}
$resources = Join-Path $destination '@Resources'
if (Test-Path -LiteralPath $resources) {
    Get-ChildItem -LiteralPath $resources -File | Where-Object {
        $_.Name -in @('DisplaySettings.inc', 'TelemetrySettings.inc') -or
        $_.Name -like '*.local.ini' -or $_.Name -like '*.local.inc'
    } | ForEach-Object { $savedSettings[$_.Name] = [IO.File]::ReadAllBytes($_.FullName) }
}
if (Test-Path -LiteralPath $destination) {
    $backup = "$destination.backup-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
    Copy-Item -LiteralPath $destination -Destination $backup -Recurse
}
$null = New-Item -ItemType Directory -Path $destination -Force
Get-ChildItem -LiteralPath $source | Copy-Item -Destination $destination -Recurse -Force
foreach ($name in $savedSettings.Keys) {
    [IO.File]::WriteAllBytes((Join-Path $resources $name), $savedSettings[$name])
}
Copy-Item -LiteralPath $plugin -Destination (Join-Path $PluginPath 'PhoneTelemetry.dll') -Force
Write-Output 'Installed PhoneMonitor. Start Rainmeter, refresh all, then load a PhoneMonitor variant.'
