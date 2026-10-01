<#
.SYNOPSIS
Launch the interactive or parameter-driven Windows library build dispatcher.

.DESCRIPTION
This stable root entry point forwards to scripts/windows/build.ps1. Run it
without arguments for the guided wizard, or pass explicit arguments for
automation.
#>
$driver = Join-Path $PSScriptRoot 'scripts\windows\build.ps1'
& $driver @args
if (-not $?) {
    exit 1
}
