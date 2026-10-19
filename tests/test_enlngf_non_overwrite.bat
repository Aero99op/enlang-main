@echo off
setlocal enabledelayedexpansion

echo =================================================================
echo   TEST: Enlngf Non-Overwrite Companion Asset Protection
echo =================================================================

set "TEST_DIR=%~dp0temp_test_non_overwrite"
if exist "%TEST_DIR%" rmdir /s /q "%TEST_DIR%"
mkdir "%TEST_DIR%"

set "CSS_FILE=%TEST_DIR%\app.css"
set "ENLNGD_FILE=%TEST_DIR%\app.enlngd"
set "ENLGF_FILE=%TEST_DIR%\sample.enlgf"
set "OUT_HTML=%TEST_DIR%\out.html"
set "ERR_LOG=%TEST_DIR%\err.log"

rem 1. Create a dummy app.css with arbitrary user CSS
echo /* My custom styles */ body { background: red; } > "%CSS_FILE%"

rem 2. Create companion app.enlngd
echo primary color is #ff0000; > "%ENLNGD_FILE%"

rem 3. Create .enlgf sample that links app.css
(
echo document enlgf:
echo     head:
echo         connect design "app.css"
echo     body:
echo         h1 "Hello Enlangg"
) > "%ENLGF_FILE%"

rem 4. Run enlngf build on the sample linking app.css
"%~dp0..\enlngf.exe" build "%ENLGF_FILE%" -o "%OUT_HTML%" 2> "%ERR_LOG%"
set "EXIT_CODE=%ERRORLEVEL%"

echo Exit Code: !EXIT_CODE!

rem 5. Verify it exits non-zero (exit code 1)
if "!EXIT_CODE!" NEQ "1" (
    echo [FAIL] Expected exit code 1, got !EXIT_CODE!
    rmdir /s /q "%TEST_DIR%"
    exit /b 1
)

rem 6. Verify exact error message in stderr
findstr /C:"Error: Companion file" "%ERR_LOG%" >nul
if errorlevel 1 (
    echo [FAIL] Error message did not contain 'Error: Companion file'
    type "%ERR_LOG%"
    rmdir /s /q "%TEST_DIR%"
    exit /b 1
)

findstr /C:"Refusing to overwrite." "%ERR_LOG%" >nul
if errorlevel 1 (
    echo [FAIL] Error message did not contain 'Refusing to overwrite.'
    type "%ERR_LOG%"
    rmdir /s /q "%TEST_DIR%"
    exit /b 1
)

rem 7. Verify the custom app.css is NOT overwritten
findstr /C:"My custom styles" "%CSS_FILE%" >nul
if errorlevel 1 (
    echo [FAIL] Custom app.css was overwritten!
    rmdir /s /q "%TEST_DIR%"
    exit /b 1
)

echo [PASS] Non-overwrite companion protection verified!
type "%ERR_LOG%"

rem Clean up
rmdir /s /q "%TEST_DIR%"
exit /b 0
