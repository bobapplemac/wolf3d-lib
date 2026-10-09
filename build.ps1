<#
.SYNOPSIS
Launch the guided or parameter-driven Windows library build system.

.DESCRIPTION
Run without arguments for the dependency-free guided configurator. Explicit
arguments are forwarded unchanged to the deterministic executor for scripts
and automation.
#>
# Git for Windows and MSYS2 supply Bash without requiring PowerShell 7 or WSL.
$git = Get-Command git -ErrorAction SilentlyContinue
if ($git -and $env:WOLF3D_GIT_CHECK -ne '0') {
    $execPath = & $git.Source --exec-path
    # MSYS2 reports a POSIX exec-path; locate Bash beside its native git.exe.
    $gitDirectory = Split-Path -Parent $git.Source
    $shell = @(
        "$gitDirectory/bash.exe",
        "$gitDirectory/../bin/bash.exe",
        "$gitDirectory/../usr/bin/bash.exe",
        "$execPath/../../../bin/bash.exe",
        "$execPath/../../../usr/bin/bash.exe",
        "$execPath/../../bin/bash.exe"
    ) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
    if ($shell) {
        $savedInteractive = $env:WOLF3D_GIT_INTERACTIVE
        try {
            if ($args -contains '-NonInteractive') { $env:WOLF3D_GIT_INTERACTIVE = '0' }
            & $shell (Join-Path $PSScriptRoot 'scripts/git-preflight.sh') $PSScriptRoot
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        } finally {
            $env:WOLF3D_GIT_INTERACTIVE = $savedInteractive
        }
    } else {
        Write-Warning 'Bash for Git was not found; building existing sources without an update check.'
    }
} elseif (-not $git) {
    Write-Warning 'Git was not found; building existing sources.'
}
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
