# Silent install / upgrade / uninstall test for installer\enlangg-setup.exe
# Touches HKCU (PATH, .enlng association, uninstall key) and restores it. Run on a dev machine, not CI.
$ErrorActionPreference = 'Stop'
$root  = Split-Path -Parent $PSScriptRoot
$setup = Join-Path $root 'installer\enlangg-setup.exe'
$tmp   = Join-Path $env:TEMP ("enlangg-inst-test-" + [guid]::NewGuid().ToString('N').Substring(0,8))
$dir   = Join-Path $tmp 'Enlangg'
$log   = Join-Path $tmp 'log.txt'
New-Item -ItemType Directory -Force $tmp | Out-Null
$pass = 0; $fail = 0
function Check($name, $cond) { if ($cond) { $script:pass++; Write-Host "PASS  $name" } else { $script:fail++; Write-Host "FAIL  $name" -ForegroundColor Red } }

function RawPath { (Get-Item 'HKCU:\Environment').GetValue('Path', $null, 'DoNotExpandEnvironmentNames') }
function AssocDefault { (Get-ItemProperty 'HKCU:\Software\Classes\.enlng' -ErrorAction SilentlyContinue).'(default)' }
function Run($argsList) { $p = Start-Process $setup -ArgumentList $argsList -Wait -PassThru; $p.ExitCode }

$pathBefore  = RawPath
$assocBefore = AssocDefault
try {
    # --- rejects bad dirs
    Check 'reject drive root'      ((Run @('/S', '/DIR=C:\', "/LOG=$log")) -ne 0)
    Check 'reject relative dir'    ((Run @('/S', '/DIR=foo\bar', "/LOG=$log")) -ne 0)
    Check 'reject ..'              ((Run @('/S', "/DIR=$tmp\a\..\b", "/LOG=$log")) -ne 0)

    # --- install
    $rc = Run @('/S', "/DIR=$dir", "/LOG=$log")
    Check 'install exit 0' ($rc -eq 0)
    foreach ($f in 'bin\enlangg.exe','bin\enlng.exe','bin\enlngdb.exe','bin\enlngf.exe','bin\enlngd.exe','bin\enlngs.exe','bin\enlngm.exe','lib\lib_std.enlng','lib\lib_math.enlng','include\enlng_runtime.h','LICENSE','uninstall.exe','install.manifest') {
        Check "file $f" (Test-Path (Join-Path $dir $f))
    }
    $bin = Join-Path $dir 'bin'
    $p1 = RawPath
    Check 'PATH starts with bin' ($p1 -like "$bin;*" -or $p1 -eq $bin)
    Check 'PATH has bin once' ((($p1 -split ';') | Where-Object { $_ -ieq $bin }).Count -eq 1)
    Check 'PATH rest unchanged' ($p1 -eq ($bin + ';' + $pathBefore) -or $p1 -eq $bin)
    Check 'assoc set' ((AssocDefault) -eq 'Enlangg.Source')
    $open = (Get-ItemProperty 'HKCU:\Software\Classes\Enlangg.Source\shell\open\command').'(default)'
    $run  = (Get-ItemProperty 'HKCU:\Software\Classes\Enlangg.Source\shell\run\command').'(default)'
    Check 'default verb does not run code' ($open -notmatch 'enlangg' -and $open -match '%1')
    Check 'run verb is separate' ($run -match 'enlangg.exe" run')
    Check 'uninstall key' (Test-Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Enlangg')
    Check 'log has gcc line' ((Get-Content $log -Raw) -match 'gcc')

    # --- installed tool works and finds its stdlib exe-relative
    $v = & (Join-Path $bin 'enlng.exe') --version 2>&1 | Out-String
    Check 'enlng --version 5.4.0' ($v -match '5\.4\.0')
    $prog = Join-Path $tmp 'hello.enlng'
    Set-Content $prog -Encoding ascii -Value @("type enlng", "", "show `"hello from installed enlangg`"")
    $out = & (Join-Path $bin 'enlangg.exe') run $prog 2>&1 | Out-String
    Check 'enlangg run works from install' ($out -match 'hello from installed enlangg')

    # --- upgrade over the same dir is idempotent
    Set-Content (Join-Path $dir 'lib\stale.enlng') 'x'
    Add-Content (Join-Path $dir 'install.manifest') "lib/stale.enlng"
    Check 'reinstall exit 0' ((Run @('/S', "/DIR=$dir", "/LOG=$log")) -eq 0)
    $p2 = RawPath
    Check 'PATH not duplicated' ((($p2 -split ';') | Where-Object { $_ -ieq $bin }).Count -eq 1)
    Check 'stale file removed on upgrade' (-not (Test-Path (Join-Path $dir 'lib\stale.enlng')))

    # --- user file in install dir survives uninstall
    Set-Content (Join-Path $dir 'mine.txt') 'keep me'

    # --- uninstall
    $u = Start-Process (Join-Path $dir 'uninstall.exe') -ArgumentList '/uninstall','/S' -Wait -PassThru
    Start-Sleep -Seconds 2
    Check 'uninstall exit 0' ($u.ExitCode -eq 0)
    Check 'bin removed' (-not (Test-Path $bin))
    Check 'lib removed' (-not (Test-Path (Join-Path $dir 'lib')))
    Check 'user file kept' (Test-Path (Join-Path $dir 'mine.txt'))
    Check 'PATH restored exactly' ((RawPath) -eq $pathBefore)
    Check 'assoc restored' ((AssocDefault) -eq $assocBefore)
    Check 'Enlangg.Source gone' (-not (Test-Path 'HKCU:\Software\Classes\Enlangg.Source'))
    Check 'uninstall key gone' (-not (Test-Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Enlangg'))
    Check 'app key gone' (-not (Test-Path 'HKCU:\Software\Enlangg'))

    # --- PATH round-trips exactly for awkward starting values (trailing ';', no value at all)
    foreach ($initial in @('C:\enlangg-test-x;', $null)) {
        $d2 = Join-Path $tmp 'P'
        if ($null -eq $initial) { Remove-ItemProperty 'HKCU:\Environment' -Name Path -ErrorAction SilentlyContinue }
        else { Set-ItemProperty 'HKCU:\Environment' -Name Path -Value $initial -Type ExpandString }
        $label = if ($null -eq $initial) { 'no PATH value' } else { "PATH '$initial'" }
        Check "scenario install ($label)" ((Run @('/S', "/DIR=$d2", "/LOG=$log")) -eq 0)
        $b2 = Join-Path $d2 'bin'
        $expect = if ($null -eq $initial) { $b2 } else { "$b2;$initial" }
        Check "scenario PATH after install ($label)" ((RawPath) -eq $expect)
        Start-Process (Join-Path $d2 'uninstall.exe') -ArgumentList '/uninstall','/S' -Wait | Out-Null
        Start-Sleep -Seconds 2
        Check "scenario PATH restored exactly ($label)" ((RawPath) -eq $initial)
    }

    # --- uninstaller refuses a copy that has no registry match
    $fake = Join-Path $tmp 'fake'; New-Item -ItemType Directory $fake | Out-Null
    Copy-Item $setup (Join-Path $fake 'uninstall.exe')
    Set-Content (Join-Path $fake 'victim.txt') 'x'; Set-Content (Join-Path $fake 'install.manifest') 'victim.txt'
    $f = Start-Process (Join-Path $fake 'uninstall.exe') -ArgumentList '/uninstall','/S' -Wait -PassThru
    Start-Sleep -Seconds 2
    Check 'uninstall of unregistered dir deletes nothing' (Test-Path (Join-Path $fake 'victim.txt'))
}
finally {
    # restore HKCU regardless of test outcome
    if ($null -ne $pathBefore) { Set-ItemProperty 'HKCU:\Environment' -Name Path -Value $pathBefore -Type ExpandString }
    else { Remove-ItemProperty 'HKCU:\Environment' -Name Path -ErrorAction SilentlyContinue }
    Start-Sleep -Milliseconds 500
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
}
Write-Host "`n$pass passed, $fail failed"
if ($fail -gt 0) { exit 1 }
