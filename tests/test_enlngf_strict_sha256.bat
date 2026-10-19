@echo off
setlocal enabledelayedexpansion

echo =================================================================
echo   TEST: Enlngf Strict SHA-256 Non-Overwrite Protection
echo =================================================================

set "TEMP_DIR=%~dp0temp_test_strict"
if exist "%TEMP_DIR%" rmdir /s /q "%TEMP_DIR%"
mkdir "%TEMP_DIR%"

copy "%~dp0..\examples\app.enlgf" "%TEMP_DIR%\app.enlgf" >nul
copy "%~dp0..\examples\app.enlgd" "%TEMP_DIR%\app.enlgd" >nul
copy "%~dp0..\examples\app.enlgs" "%TEMP_DIR%\app.enlgs" >nul

:: -------------------------------------------------------------------
:: SCENARIO 1: User file containing the OLD header ("Compiled by Enlangg")
:: Must REFUSE with exit code 1 and must NOT overwrite the file!
:: -------------------------------------------------------------------
echo.
echo [SCENARIO 1] User file containing the OLD header...
(
echo /**
echo  * Compiled by Enlangg Sovereign Design Token Engine v5.3.0
echo  */
echo body { background: #ff00ff; }
) > "%TEMP_DIR%\app.css"

"%~dp0..\enlngf.exe" build "%TEMP_DIR%\app.enlgf" -o "%TEMP_DIR%\out.html" >nul 2> "%TEMP_DIR%\err1.log"
set "RET1=%ERRORLEVEL%"
if %RET1% EQU 1 (
    echo   [PASS] Scenario 1: Refused with exit code 1
    type "%TEMP_DIR%\err1.log"
) else (
    echo   [FAIL] Scenario 1: Expected exit code 1, got %RET1%
    exit /b 1
)

findstr /C:"#ff00ff" "%TEMP_DIR%\app.css" >nul
if %ERRORLEVEL% EQU 0 (
    echo   [PASS] User file content with old header preserved intact
) else (
    echo   [FAIL] User file was overwritten!
    exit /b 1
)

:: -------------------------------------------------------------------
:: SCENARIO 2: Fresh build (app.css does not exist)
:: Must succeed with exit code 0 and stamp /* enlngf-generated: <sha256> */
:: -------------------------------------------------------------------
echo.
echo [SCENARIO 2] Fresh build with no pre-existing app.css...
del "%TEMP_DIR%\app.css" 2>nul
del "%TEMP_DIR%\app.js" 2>nul

"%~dp0..\enlngf.exe" build "%TEMP_DIR%\app.enlgf" -o "%TEMP_DIR%\out.html"
set "RET2=%ERRORLEVEL%"
if %RET2% EQU 0 (
    echo   [PASS] Scenario 2: Build succeeded with exit code 0
) else (
    echo   [FAIL] Scenario 2: Build failed with exit code %RET2%
    exit /b 1
)

findstr /C:"/* enlngf-generated:" "%TEMP_DIR%\app.css" >nul
if %ERRORLEVEL% EQU 0 (
    echo   [PASS] Found '/* enlngf-generated:' marker in generated app.css
) else (
    echo   [FAIL] Marker missing from generated app.css!
    exit /b 1
)

:: -------------------------------------------------------------------
:: SCENARIO 3: Rebuild when marker is valid and matches content
:: Must succeed with exit code 0
:: -------------------------------------------------------------------
echo.
echo [SCENARIO 3] Rebuilding when generated marker matches content...
"%~dp0..\enlngf.exe" build "%TEMP_DIR%\app.enlgf" -o "%TEMP_DIR%\out.html"
set "RET3=%ERRORLEVEL%"
if %RET3% EQU 0 (
    echo   [PASS] Scenario 3: Allowed overwrite of valid generated asset [exit 0]
) else (
    echo   [FAIL] Scenario 3: Erroneously refused valid generated asset [exit %RET3%]
    exit /b 1
)

:: -------------------------------------------------------------------
:: SCENARIO 4: Tampered/modified generated asset
:: User tweaks generated CSS -> marker hash no longer matches content
:: Must REFUSE with exit code 1!
:: -------------------------------------------------------------------
echo.
echo [SCENARIO 4] Modified generated asset (tampered content)...
echo /* User added custom modification */ >> "%TEMP_DIR%\app.css"

"%~dp0..\enlngf.exe" build "%TEMP_DIR%\app.enlgf" -o "%TEMP_DIR%\out.html" >nul 2> "%TEMP_DIR%\err4.log"
set "RET4=%ERRORLEVEL%"
if %RET4% EQU 1 (
    echo   [PASS] Scenario 4: Refused tampered asset with exit code 1
    type "%TEMP_DIR%\err4.log"
) else (
    echo   [FAIL] Scenario 4: Expected exit code 1, got %RET4%
    exit /b 1
)

rmdir /s /q "%TEMP_DIR%"
echo.
echo =================================================================
echo   ALL STRICT SHA-256 NON-OVERWRITE SCENARIOS PASSED (100%%)
echo =================================================================
exit /b 0
