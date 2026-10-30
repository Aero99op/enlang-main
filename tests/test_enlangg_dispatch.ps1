# tests/test_enlangg_dispatch.ps1
#
# Proves that enlangg runs enlng/enlngf/enlngd/enlngs/enlngm from its own
# directory first, and only falls back to PATH when that file is missing.
#
#   -BinDir  folder containing the 7 freshly built toolchain .exe files.
#
# Layout created under %TEMP%\enlangg-dispatch-<guid>:
#   toolchain\bin\   copy of the real tools (NOT on PATH)
#   fakepath\        impostor enlng.exe, enlngf.exe, ... (FIRST on PATH)
#   work\            current directory; also holds an impostor enlng.exe
#
# Cases:
#   A  real tools present, impostors first on PATH and in CWD -> real tools run
#   B  real enlng.exe removed                                  -> PATH impostor runs (fallback)
#   C  real enlng.exe removed, impostors removed from PATH     -> clear error, CWD impostor ignored
#   D  install folder path contains spaces                     -> real tools run
param(
    [Parameter(Mandatory = $true)][string]$BinDir
)
$ErrorActionPreference = 'Stop'
$failures = 0
function Pass([string]$m) { Write-Host "  [PASS] $m" }
function Fail([string]$m) { Write-Host "  [FAIL] $m"; $script:failures++ }
function Check([bool]$c, [string]$m) { if ($c) { Pass $m } else { Fail $m } }

$BinDir = (Resolve-Path $BinDir).Path
$tools = @('enlangg', 'enlng', 'enlngdb', 'enlngf', 'enlngd', 'enlngs', 'enlngm')
foreach ($t in $tools) {
    if (-not (Test-Path (Join-Path $BinDir "$t.exe"))) { throw "Missing $t.exe in $BinDir" }
}

$root = Join-Path $env:TEMP ("enlangg-dispatch-" + [guid]::NewGuid().ToString('N').Substring(0, 8))
$real = Join-Path $root 'toolchain\bin'
$fake = Join-Path $root 'fakepath'
$work = Join-Path $root 'work'
$spaced = Join-Path $root 'Program Files Test\Enlangg\bin'
New-Item -ItemType Directory -Force -Path $real, $fake, $work, $spaced | Out-Null
Copy-Item (Join-Path $BinDir '*.exe') $real
Copy-Item (Join-Path $BinDir '*.exe') $spaced

$fixture = Join-Path $PSScriptRoot 'fixtures\fake_tool.c'
foreach ($t in @('enlng', 'enlngf', 'enlngd', 'enlngs', 'enlngm')) {
    & gcc -O0 $fixture -o (Join-Path $fake "$t.exe")
    if ($LASTEXITCODE -ne 0) { throw "failed to build impostor $t.exe" }
}
Copy-Item (Join-Path $fake 'enlng.exe') (Join-Path $work 'enlng.exe')

# PATH = impostors first, then the original PATH minus any folder that
# already contains an Enlangg tool (so no other install can interfere).
# gcc must stay reachable because enlng compiles through it.
$origPath = $env:PATH
$kept = @()
foreach ($e in ($origPath -split ';')) {
    if (-not $e) { continue }
    $hasTool = $false
    foreach ($t in $tools) { if (Test-Path (Join-Path $e.Trim('"') "$t.exe")) { $hasTool = $true } }
    if (-not $hasTool) { $kept += $e }
}
$env:PATH = (@($fake) + $kept) -join ';'
Remove-Item Env:\ENLANGG_HOME -ErrorAction SilentlyContinue

Set-Content -Path (Join-Path $work 'hello.enlng') -Encoding ASCII -Value @(
    'type enlng',
    'remember a as 6',
    'remember b as 7',
    'remember c as a * b',
    'show c'
)

function Run([string]$exe, [string[]]$argv) {
    $prev = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $out = & $exe @argv 2>&1 | ForEach-Object { "$_" }
        $code = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $prev
    }
    return [pscustomobject]@{ Code = $code; Out = ($out -join "`n") }
}

Push-Location $work
try {
    Write-Host "================================================================="
    Write-Host "  TEST: enlangg sibling-first tool dispatch (Windows)"
    Write-Host "================================================================="
    $resolved = (Get-Command enlng -CommandType Application | Select-Object -First 1).Source
    Write-Host "  'enlng' as PATH would resolve it : $($resolved.Replace($root, '<tmp>'))"
    Write-Host "  toolchain bin on PATH            : $([bool](($env:PATH -split ';') -contains $real))"
    Check ($resolved -like "$fake*") "precondition: an impostor enlng.exe is first on PATH"
    Check (-not (($env:PATH -split ';') -contains $real)) "precondition: real toolchain bin is not on PATH"

    Write-Host "`n[A] Real tools next to enlangg; impostors on PATH and in CWD"
    $r = Run (Join-Path $real 'enlangg.exe') @('run', 'hello.enlng')
    Write-Host "  enlangg run hello.enlng -> exit $($r.Code)"
    Write-Host ($r.Out -split "`n" | ForEach-Object { "    | $_" } | Out-String).TrimEnd()
    Check ($r.Code -eq 0 -and $r.Out.Trim() -eq '42') "enlng sibling ran the program (output 42, exit 0)"
    Check ($r.Out -notmatch 'FAKE-TOOL') "no impostor output"
    foreach ($pair in @(@('f', 'enlngf'), @('d', 'enlngd'), @('s', 'enlngs'), @('m', 'enlngm'), @('db', 'enlngdb'), @('enlng', 'enlng'))) {
        $r = Run (Join-Path $real 'enlangg.exe') @($pair[0], '--version')
        Write-Host "  enlangg $($pair[0]) --version -> exit $($r.Code): $($r.Out.Trim())"
        Check ($r.Code -eq 0 -and $r.Out -match "^$($pair[1]) version 5.5.1" -and $r.Out -notmatch 'FAKE-TOOL') "$($pair[1]) dispatched to the sibling/embedded engine"
    }

    Write-Host "`n[B] Sibling enlng.exe removed -> PATH fallback"
    Remove-Item (Join-Path $real 'enlng.exe')
    $r = Run (Join-Path $real 'enlangg.exe') @('run', 'hello.enlng')
    Write-Host "  enlangg run hello.enlng -> exit $($r.Code)"
    Write-Host ($r.Out -split "`n" | ForEach-Object { "    | $_" } | Out-String).TrimEnd()
    Check ($r.Out -match 'FAKE-TOOL-FROM-PATH enlng\.exe') "fell back to the enlng.exe found on PATH"
    Check ($r.Code -eq 1) "impostor's non-zero exit reported as 1"

    Write-Host "`n[C] Sibling missing and nothing on PATH (impostor still in CWD)"
    $env:PATH = $kept -join ';'
    $r = Run (Join-Path $real 'enlangg.exe') @('run', 'hello.enlng')
    Write-Host "  enlangg run hello.enlng -> exit $($r.Code)"
    Write-Host ($r.Out.Replace($root, '<tmp>') -split "`n" | ForEach-Object { "    | $_" } | Out-String).TrimEnd()
    Check ($r.Code -eq 1) "exit 1"
    Check ($r.Out -match "could not find 'enlng\.exe' next to enlangg") "clear 'could not find' error"
    Check ($r.Out -notmatch 'FAKE-TOOL') "CWD impostor was not executed"

    Write-Host "`n[D] Install folder with spaces in its path"
    $env:PATH = (@($fake) + $kept) -join ';'
    $r = Run (Join-Path $spaced 'enlangg.exe') @('run', 'hello.enlng')
    Write-Host "  <tmp>\Program Files Test\Enlangg\bin\enlangg.exe run hello.enlng -> exit $($r.Code): $($r.Out.Trim())"
    Check ($r.Code -eq 0 -and $r.Out.Trim() -eq '42') "sibling found and run from a path with spaces"
}
finally {
    Pop-Location
    $env:PATH = $origPath
    Remove-Item -Recurse -Force $root -ErrorAction SilentlyContinue
}

Write-Host "`n================================================================="
if ($failures -eq 0) { Write-Host "  ALL DISPATCH CHECKS PASSED" } else { Write-Host "  $failures CHECK(S) FAILED" }
Write-Host "================================================================="
exit ([int]($failures -ne 0))
