@echo off
setlocal enabledelayedexpansion

echo =====================================================================
echo     ENLANGG SOVEREIGN 7-IN-1 SUITE - Command Prompt (CMD) Installer
echo     Pure C Zero-Python Native Architecture
echo =====================================================================

set "INSTALL_DIR=%USERPROFILE%\.enlangg\bin"
if not exist "%INSTALL_DIR%" (
    mkdir "%INSTALL_DIR%"
)

set "PRIMARY_URL=https://enlangg.vercel.app"

echo [1/3] Installing 7 Sovereign Pure C Executables to: %INSTALL_DIR%
echo   - enlangg.exe (Universal Toolchain Dispatcher)
echo   - enlng.exe   (Core Language Compiler and Scoped Arena Memory)
echo   - enlngdb.exe (Pure C Microsecond Database Engine)
echo   - enlngf.exe  (Pure C Desktop Window GUI and WinSock2 Web Studio)
echo   - enlngd.exe  (Pure C Design Tokens and Style Resolver)
echo   - enlngs.exe  (Pure C In-Memory Reactive Script VM)
echo   - enlngm.exe  (Pure C Smartphone Simulator and HAL Packager)

if exist "%~dp0enlangg.exe" (
    if exist "%~dp0enlng.exe" (
        if exist "%~dp0enlngdb.exe" (
            echo   Copying local build binaries...
            for %%B in (enlangg.exe enlng.exe enlngdb.exe enlngf.exe enlngd.exe enlngs.exe enlngm.exe) do (
                if exist "%~dp0%%B" copy /y "%~dp0%%B" "%INSTALL_DIR%\%%B" >nul
            )
            goto :after_copy
        )
    )
)

echo   Downloading prebuilt release archive and SHA256SUMS from GitHub...
set "RELEASE_BASE=https://github.com/Aero99op/enlang-main/releases/latest/download"
set "ARCHIVE=enlangg-windows-amd64.zip"
set "TMP_DIR=%TEMP%\enlangg_install_%RANDOM%"
mkdir "%TMP_DIR%" >nul 2>&1

curl -fsSL "%RELEASE_BASE%/SHA256SUMS" -o "%TMP_DIR%\SHA256SUMS"
if !errorlevel! neq 0 (
    echo [ERROR] Failed to download SHA256SUMS from %RELEASE_BASE%.
    rmdir /s /q "%TMP_DIR%"
    exit /b 1
)

curl -fsSL "%RELEASE_BASE%/%ARCHIVE%" -o "%TMP_DIR%\%ARCHIVE%"
if !errorlevel! neq 0 (
    echo [ERROR] Failed to download %ARCHIVE% from %RELEASE_BASE%.
    rmdir /s /q "%TMP_DIR%"
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$exp = (Get-Content '%TMP_DIR%\SHA256SUMS' | Select-String '%ARCHIVE%').Line.Split()[0].ToLower(); " ^
  "$act = (Get-FileHash '%TMP_DIR%\%ARCHIVE%' -Algorithm SHA256).Hash.ToLower(); " ^
  "if (-not $exp -or $exp -ne $act) { Write-Error ('Checksum mismatch: exp=' + $exp + ' act=' + $act); exit 1 }"
if !errorlevel! neq 0 (
    echo [ERROR] SHA-256 verification failed for %ARCHIVE%!
    rmdir /s /q "%TMP_DIR%"
    exit /b 1
)
echo   [OK] SHA-256 verified successfully.

powershell -NoProfile -Command "Expand-Archive -Path '%TMP_DIR%\%ARCHIVE%' -DestinationPath '%TMP_DIR%\out' -Force"
for %%B in (enlangg.exe enlng.exe enlngdb.exe enlngf.exe enlngd.exe enlngs.exe enlngm.exe enlangg-bridge.py enlangg-bridge.cmd) do (
    if exist "%TMP_DIR%\out\%%B" copy /y "%TMP_DIR%\out\%%B" "%INSTALL_DIR%\%%B" >nul
)
rmdir /s /q "%TMP_DIR%"

:after_copy

echo [2/3] Configuring system PATH environment variable...
echo %PATH% | find /i "%INSTALL_DIR%" >nul
if %errorlevel% equ 0 (
    echo   [OK] %INSTALL_DIR% is already in PATH.
) else (
    echo   Adding %INSTALL_DIR% to User PATH via setx...
    setx PATH "%PATH%;%INSTALL_DIR%" >nul
    echo   [OK] Added to User PATH.
)

set "PATH=%PATH%;%INSTALL_DIR%"

echo [3/3] Verifying 7-in-1 Suite installation:
"%INSTALL_DIR%\enlangg.exe" --version
"%INSTALL_DIR%\enlng.exe" --version
"%INSTALL_DIR%\enlngdb.exe" --version
"%INSTALL_DIR%\enlngf.exe" --version
"%INSTALL_DIR%\enlngd.exe" --version
"%INSTALL_DIR%\enlngs.exe" --version
"%INSTALL_DIR%\enlngm.exe" --version

echo =====================================================================
echo   [SUCCESS] Complete Enlangg 7-in-1 Suite installed successfully!
echo =====================================================================
echo Available Pure C Sovereign Commands:
echo   enlangg [file]                    (Universal Toolchain Dispatcher)
echo   enlng run [file.enlng]            (Core Backend Computing)
echo   enlngdb [script.enlngdb]          (Pure C Microsecond Database)
echo   enlngs [logic.enlngs]             (Pure C In-Memory Script VM)
echo   enlngf [app.enlngf]               (Pure C Desktop Window GUI)
echo   enlngf serve [app.enlngf]         (Pure C WinSock2 Web Studio)
echo   enlngd [theme.enlngd]             (Pure C Design Tokens and Styles)
echo   enlngm [app.enlngm]               (Pure C Smartphone Simulator)
echo   enlngm build [app.enlngm]         (Pure C Mobile HAL Packager)
echo.
echo Documentation and Online Playground: https://enlangg.vercel.app
