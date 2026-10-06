<#
.SYNOPSIS
Launch the guided or parameter-driven Windows library build system.

.DESCRIPTION
Run without arguments for the dependency-free guided configurator. Explicit
arguments are forwarded unchanged to the deterministic executor for scripts
and automation.
#>
$driver = if ($args.Count -eq 0) {
    Join-Path $PSScriptRoot 'scripts\windows\configure-build.ps1'
} else {
    Join-Path $PSScriptRoot 'scripts\windows\invoke-build.ps1'
}
Push-Location -LiteralPath $PSScriptRoot
try {
    & $driver @args
    if (-not $?) {
        exit 1
    }
} finally {
    Pop-Location
}
