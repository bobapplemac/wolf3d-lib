<#
.SYNOPSIS
Build one wolf3d-lib Windows configuration with an installed Visual Studio toolchain.

.DESCRIPTION
This modern Windows dispatcher detects supported Visual Studio installations
and maps human-readable compiler, architecture, runtime, audio, OPL, and action
choices onto the authoritative CMake presets and targets.

.EXAMPLE
.\build.ps1
Packages the x64 SDK with the newest supported installed compiler.

.EXAMPLE
.\build.ps1 -Compiler vs2019 -Architecture x86 -Action test
Builds and tests the 32-bit library with Visual Studio 2019/v142.

.EXAMPLE
.\build.ps1 -Action test -Audio silent
Validates the timing-preserving silent backend.

.EXAMPLE
.\build.ps1 -Action package -Opl dbopl -Runtime dynamic
Stages a DBOPL SDK linked to the dynamic MSVC runtime.
#>
[CmdletBinding()]
param(
    [ValidateSet('auto', 'vs2022', 'vs2019')]
    [string]$Compiler = 'auto',
    [ValidateSet('x64', 'x86')]
    [string]$Architecture = 'x64',
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release',
    [ValidateSet('static', 'dynamic')]
    [string]$Runtime = 'static',
    [ValidateSet('standard', 'silent')]
    [string]$Audio = 'standard',
    [ValidateSet('nuked', 'dbopl')]
    [string]$Opl = 'nuked',
    [ValidateSet('package', 'build', 'test', 'clean')]
    [string]$Action = 'package',
    [ValidateRange(0, 256)]
    [int]$Jobs = 0,
    [switch]$List,
    [switch]$DryRun,
    [switch]$NonInteractive
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

function Find-VisualStudio {
    param(
        [string]$Name,
        [string]$VersionRange,
        [string]$FallbackPath,
        [string]$Toolset,
        [string]$PresetPrefix
    )

    $installation = $null
    if (Test-Path -LiteralPath $vswhere) {
        $result = & $vswhere -latest -products '*' -version $VersionRange `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
        if ($LASTEXITCODE -eq 0 -and $result) {
            $installation = ($result | Select-Object -Last 1).Trim()
        }
    }
    if (-not $installation -and (Test-Path -LiteralPath $FallbackPath)) {
        $installation = $FallbackPath
    }

    $cmake = $null
    $ctest = $null
    if ($installation) {
        $cmakeCandidate = Join-Path $installation `
            'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
        $ctestCandidate = Join-Path $installation `
            'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
        if (Test-Path -LiteralPath $cmakeCandidate) {
            $cmake = $cmakeCandidate
        }
        if (Test-Path -LiteralPath $ctestCandidate) {
            $ctest = $ctestCandidate
        }
    }

    [pscustomobject]@{
        Name = $Name
        Toolset = $Toolset
        PresetPrefix = $PresetPrefix
        Installation = $installation
        CMake = $cmake
        CTest = $ctest
        Available = [bool]($installation -and $cmake -and $ctest)
    }
}

$toolchains = @(
    Find-VisualStudio -Name 'vs2022' -VersionRange '[17.0,18.0)' `
        -FallbackPath 'C:\Program Files\Microsoft Visual Studio\2022\Enterprise' `
        -Toolset 'v143' -PresetPrefix 'windows-vs2022'
    Find-VisualStudio -Name 'vs2019' -VersionRange '[16.0,17.0)' `
        -FallbackPath 'C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise' `
        -Toolset 'v142' -PresetPrefix 'windows'
)

function Read-BuildChoice {
    param(
        [string]$Prompt,
        [string[]]$Options,
        [int]$DefaultIndex = 0
    )

    while ($true) {
        Write-Host ''
        Write-Host $Prompt
        for ($index = 0; $index -lt $Options.Count; ++$index) {
            $defaultMarker = if ($index -eq $DefaultIndex) { ' (default)' } else { '' }
            Write-Host ("  {0}. {1}{2}" -f ($index + 1), $Options[$index], $defaultMarker)
        }
        $answer = Read-Host 'Selection'
        if ([string]::IsNullOrWhiteSpace($answer)) {
            return $Options[$DefaultIndex]
        }
        $number = 0
        if ([int]::TryParse($answer, [ref]$number) -and
            $number -ge 1 -and $number -le $Options.Count) {
            return $Options[$number - 1]
        }
        $match = $Options | Where-Object { $_ -ieq $answer } | Select-Object -First 1
        if ($match) { return $match }
        Write-Warning 'Enter a listed number or name.'
    }
}

$canPrompt = [Environment]::UserInteractive
try { $canPrompt = $canPrompt -and -not [Console]::IsInputRedirected } catch {}
if ($PSBoundParameters.Count -eq 0 -and -not $NonInteractive -and $canPrompt) {
    Write-Host 'wolf3d-lib guided Windows build'
    Write-Host 'Press Enter to accept each default.'
    $availableCompilers = @($toolchains | Where-Object { $_.Available } |
        ForEach-Object { $_.Name })
    if ($availableCompilers.Count -eq 0) {
        throw 'No supported Visual Studio installation was detected.'
    }
    $Compiler = Read-BuildChoice 'Compiler' $availableCompilers
    $Architecture = Read-BuildChoice 'Architecture' @('x64', 'x86')
    $Action = Read-BuildChoice 'Action' @('package', 'build', 'test', 'clean')
    if ($Action -ne 'package') {
        $Configuration = Read-BuildChoice 'Configuration' @('Release', 'Debug')
    }
    $Runtime = Read-BuildChoice 'MSVC runtime' @('static', 'dynamic')
    $Audio = Read-BuildChoice 'Audio backend' @('standard', 'silent')
    if ($Audio -eq 'standard') {
        $Opl = Read-BuildChoice 'OPL emulator' @('nuked', 'dbopl')
    }
    Write-Host ''
    $confirmation = Read-Host 'Continue with this build? [Y/n]'
    if ($confirmation -and $confirmation -notmatch '^[Yy]') { exit 0 }
}

if ($List) {
    $toolchains | Select-Object Name, Toolset, Available, Installation | Format-Table -AutoSize
    exit 0
}
if ($Action -eq 'package' -and $Configuration -ne 'Release') {
    throw 'Packaging is restricted to Release builds. Use -Action build or test for Debug.'
}
if ($Audio -eq 'silent' -and $Opl -ne 'nuked') {
    Write-Warning '-Opl is ignored by the silent audio backend.'
    $Opl = 'nuked'
}

if ($Compiler -eq 'auto') {
    $selected = $toolchains | Where-Object { $_.Available } | Select-Object -First 1
} else {
    $selected = $toolchains | Where-Object { $_.Name -eq $Compiler } | Select-Object -First 1
}
if (-not $selected -or -not $selected.Available) {
    $requested = if ($Compiler -eq 'auto') { 'a supported Visual Studio installation' } else { $Compiler }
    throw "Could not find $requested with C++ tools and bundled CMake. Run .\build.ps1 -List."
}

$archPreset = if ($Architecture -eq 'x86') { 'x86' } else { 'x64' }
$devPreset = "$($selected.PresetPrefix)-dev-$archPreset"
$libraryPreset = "$($selected.PresetPrefix)-library-$archPreset"
$useDedicatedPreset = ($Action -eq 'package' -or $Action -eq 'clean') `
    -and $Runtime -eq 'static' -and $Audio -eq 'standard' -and $Opl -eq 'nuked'
$preset = if ($useDedicatedPreset) { $libraryPreset } else { $devPreset }

if ($useDedicatedPreset) {
    $buildDir = $null
} else {
    $parts = @('windows', $selected.Name, 'dispatch', 'library', $archPreset)
    if ($Configuration -eq 'Debug') { $parts += 'debug' }
    if ($Runtime -eq 'dynamic') { $parts += 'dynamic-crt' }
    if ($Audio -eq 'silent') { $parts += 'silent' }
    elseif ($Opl -ne 'nuked') { $parts += $Opl }
    $buildDir = Join-Path $root ('build\' + ($parts -join '-'))
}

function Invoke-DisplayedCommand {
    param([string]$Executable, [string[]]$Arguments)
    $displayArguments = $Arguments | ForEach-Object {
        if ($_ -match '[\s"]') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ }
    }
    Write-Host ('> "{0}" {1}' -f $Executable, ($displayArguments -join ' '))
    if (-not $DryRun) {
        & $Executable @Arguments
        if ($LASTEXITCODE -ne 0) { throw "Command failed with exit code $LASTEXITCODE." }
    }
}

Write-Host 'wolf3d-lib Windows build'
Write-Host "  Compiler:      $($selected.Name) / $($selected.Toolset)"
Write-Host "  Architecture:  $Architecture"
Write-Host "  Configuration: $Configuration"
Write-Host "  CRT:           $Runtime"
Write-Host "  Audio:         $Audio"
Write-Host "  OPL:           $Opl"
Write-Host "  Action:        $Action"

if ($Action -eq 'clean' -and $buildDir -and
    -not (Test-Path -LiteralPath (Join-Path $buildDir 'CMakeCache.txt'))) {
    Write-Host "Nothing to clean: $buildDir has not been configured."
    exit 0
}

if ($Action -ne 'clean') {
    $configureArguments = @('--preset', $preset)
    if ($buildDir) {
        $configureArguments += @(
            '-B', $buildDir,
            "-DWG_STATIC_MSVC_RUNTIME=$(if ($Runtime -eq 'static') { 'ON' } else { 'OFF' })",
            "-DWG_AUDIO_BACKEND=$Audio",
            "-DWG_OPL_BACKEND=$Opl"
        )
    }
    Invoke-DisplayedCommand -Executable $selected.CMake -Arguments $configureArguments
}

$buildArguments = @('--build')
if ($buildDir) { $buildArguments += $buildDir }
else { $buildArguments += @('--preset', $preset) }
$buildArguments += @('--config', $Configuration)
if ($Action -eq 'clean') {
    $buildArguments += @('--target', 'clean')
} elseif (-not $useDedicatedPreset) {
    if ($Action -eq 'package') { $buildArguments += @('--target', 'library-release') }
    elseif ($Action -eq 'build') { $buildArguments += @('--target', 'wolf3d') }
}
if ($Jobs -gt 0) { $buildArguments += @('--parallel', $Jobs.ToString()) }
Invoke-DisplayedCommand -Executable $selected.CMake -Arguments $buildArguments

if ($Action -eq 'test') {
    Invoke-DisplayedCommand -Executable $selected.CTest -Arguments @(
        '--test-dir', $buildDir, '-C', $Configuration, '--output-on-failure')
} elseif ($Action -eq 'package') {
    Write-Host "Published SDK is under $root\dist."
}
