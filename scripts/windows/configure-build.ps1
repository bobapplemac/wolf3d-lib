<#
.SYNOPSIS
Interactively configure and run a wolf3d-lib Windows build.

.DESCRIPTION
This dependency-free frontend detects supported local toolchains, asks only
questions relevant to the selected compiler, displays the reproducible
executor command, and optionally runs it. It never installs external tools.
#>
[CmdletBinding()]
param([string]$Msys2Root = 'C:\msys64')

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$executor = Join-Path $PSScriptRoot 'invoke-build.ps1'

function Read-Choice {
    param([string]$Prompt, [object[]]$Options, [string]$LabelProperty = '')
    while ($true) {
        Write-Host ''
        Write-Host $Prompt
        for ($i = 0; $i -lt $Options.Count; ++$i) {
            $label = if ($LabelProperty) { $Options[$i].$LabelProperty } else { [string]$Options[$i] }
            $marker = if ($i -eq 0) { ' (recommended)' } else { '' }
            Write-Host ('  {0}. {1}{2}' -f ($i + 1), $label, $marker)
        }
        $answer = Read-Host 'Selection'
        if ([string]::IsNullOrWhiteSpace($answer)) { return $Options[0] }
        $number = 0
        if ([int]::TryParse($answer, [ref]$number) -and
            $number -ge 1 -and $number -le $Options.Count) {
            return $Options[$number - 1]
        }
        Write-Warning 'Enter one of the listed numbers.'
    }
}

function Confirm-Step {
    param([string]$Prompt, [bool]$DefaultYes = $true)
    $suffix = if ($DefaultYes) { '[Y/n]' } else { '[y/N]' }
    $answer = Read-Host "$Prompt $suffix"
    if ([string]::IsNullOrWhiteSpace($answer)) { return $DefaultYes }
    return $answer -match '^[Yy]'
}

Write-Host 'wolf3d-lib guided Windows build'
Write-Host 'Scanning supported compilers and build tools...'
$toolchains = @(& $executor -ListObjects -NonInteractive -Msys2Root $Msys2Root)
$available = @($toolchains | Where-Object { $_.Available })

Write-Host ''
Write-Host 'Detected build environments:'
foreach ($toolchain in $toolchains) {
    $status = if ($toolchain.Available) { 'ready' } else { 'not found/incomplete' }
    $location = if ($toolchain.Installation) { " - $($toolchain.Installation)" } else { '' }
    Write-Host ('  {0,-14} {1,-12} {2}{3}' -f $toolchain.Name, $toolchain.Toolset, $status, $location)
}
if ($available.Count -eq 0) {
    throw 'No complete supported Windows build environment was detected. See docs/building.md for prerequisite details.'
}

$compiler = Read-Choice 'Compiler' $available 'Name'
$architectures = if ($compiler.Name -eq 'mingw-ucrt64') { @('x64') } else { @('x64', 'x86') }
$architecture = Read-Choice 'Architecture' $architectures
$action = Read-Choice 'What would you like to produce?' @('package', 'test', 'build', 'clean')
$configuration = if ($action -eq 'package') { 'Release' } else {
    Read-Choice 'Configuration' @('Release', 'Debug')
}
$runtime = if ($compiler.Name -eq 'mingw-ucrt64') { 'static' } else {
    Read-Choice 'Compiler runtime' @('static', 'dynamic')
}
$drivers = Read-Choice 'Compiled OPL drivers' @('all', 'nuked-dbopl', 'nuked-silent', 'dbopl-silent', 'nuked', 'dbopl', 'silent')
$availableDefaults = if ($drivers -eq 'all') { @('nuked', 'dbopl', 'silent') } else { @($drivers -split '-') }
$defaultOpl = Read-Choice 'Default OPL driver' $availableDefaults
$sampleRate = Read-Choice 'Preferred PCM sample rate' @('48000', '44100')

$commandArguments = @(
    '-Compiler', $compiler.Name,
    '-Architecture', $architecture,
    '-Configuration', $configuration,
    '-Runtime', $runtime,
    '-Drivers', $drivers,
    '-DefaultOpl', $defaultOpl,
    '-SampleRate', $sampleRate,
    '-Action', $action,
    '-NonInteractive'
)
if ($compiler.Name -eq 'mingw-ucrt64') { $commandArguments += @('-Msys2Root', $Msys2Root) }
$parameters = @{
    Compiler = $compiler.Name
    Architecture = $architecture
    Configuration = $configuration
    Runtime = $runtime
    Drivers = $drivers
    DefaultOpl = $defaultOpl
    SampleRate = [int]$sampleRate
    Action = $action
    NonInteractive = $true
}
if ($compiler.Name -eq 'mingw-ucrt64') { $parameters.Msys2Root = $Msys2Root }

Write-Host ''
Write-Host 'Build plan:'
Write-Host "  Compiler:      $($compiler.Name) / $($compiler.Toolset)"
Write-Host "  Architecture:  $architecture"
Write-Host "  Result:        $action / $configuration"
Write-Host "  Runtime:       $runtime"
Write-Host "  OPL drivers:   $drivers (default: $defaultOpl)"
Write-Host "  Sample rate:   $sampleRate Hz"
Write-Host ''
Write-Host ('Reproducible command:')
Write-Host ('.\scripts\windows\invoke-build.ps1 ' + ($commandArguments -join ' '))
Write-Host ''
if (-not (Confirm-Step 'Run this build now?')) { exit 0 }

& $executor @parameters
if (-not $?) { exit 1 }
