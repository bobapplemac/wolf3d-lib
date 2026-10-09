param([string]$Msys2Root = 'C:\msys64')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$executor = Join-Path $root 'scripts\windows\invoke-build.ps1'
$before = $env:PATH
try {
    # A real MinGW dry run must leave the calling terminal unchanged.
    & $executor -Compiler mingw-ucrt64 -Architecture x64 -Msys2Root $Msys2Root -DryRun -NonInteractive
    if ($env:PATH -cne $before) { throw 'Dry run changed PATH.' }

    # Exercise the actual invocation function without running a full build.
    $tokens = $null
    $parseErrors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile($executor, [ref]$tokens, [ref]$parseErrors)
    if ($parseErrors.Count) { throw 'Build script did not parse.' }
    $commandFunction = $ast.Find({ param($node)
        $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and
        $node.Name -eq 'Invoke-DisplayedCommand'
    }, $true)
    Invoke-Expression $commandFunction.Extent.Text
    $selected = [pscustomobject]@{
        Name = 'mingw-ucrt64'
        Bin = Join-Path $Msys2Root 'ucrt64\bin'
        Installation = $Msys2Root
    }
    $DryRun = $false
    function Test-BuildCommand {
        if (-not $env:PATH.StartsWith($selected.Bin + ';')) {
            throw 'Build command did not receive MinGW PATH.'
        }
        if ($script:failCommand) { throw 'Intentional build failure.' }
        $global:LASTEXITCODE = 0
    }
    foreach ($failure in @($false, $true)) {
        $script:failCommand = $failure
        $caught = $false
        try { Invoke-DisplayedCommand -Executable 'Test-BuildCommand' -Arguments @() }
        catch {
            if (-not $failure -or $_ -notmatch 'Intentional build failure') { throw }
            $caught = $true
        }
        if ($failure -and -not $caught) { throw 'Expected command failure.' }
        if ($env:PATH -cne $before) { throw 'Build command leaked PATH.' }
    }
    # Native nonzero exits must also restore the environment.
    $caught = $false
    try { Invoke-DisplayedCommand -Executable $env:ComSpec -Arguments @('/d', '/c', 'exit 7') }
    catch {
        if ($_ -notmatch 'exit code 7') { throw }
        $caught = $true
    }
    if (-not $caught -or $env:PATH -cne $before) { throw 'Native failure leaked PATH or was ignored.' }
    Write-Host 'PASS: dry run, success, exception, and native failure preserve caller PATH.'
} finally {
    $env:PATH = $before
}
