@echo off
setlocal enabledelayedexpansion

echo =====================================================================
echo     ENLANGG ^& ENLNG - Command Prompt (CMD) Windows Installer
echo =====================================================================

set "INSTALL_DIR=%USERPROFILE%\.enlangg\bin"
if not exist "%INSTALL_DIR%" (
    mkdir "%INSTALL_DIR%"
)

set "PRIMARY_URL=https://enlangg.vercel.app"

echo [1/3] Installing enlangg ^& enlng executables to: %INSTALL_DIR%

if exist "%~dp0enlangg.exe" (
    if exist "%~dp0enlng.exe" (
        echo   Copying local build binaries...
        copy /y "%~dp0enlangg.exe" "%INSTALL_DIR%\enlangg.exe" >nul
        copy /y "%~dp0enlng.exe" "%INSTALL_DIR%\enlng.exe" >nul
        goto :after_copy
    )
)

echo   Downloading sovereign production binaries...
curl -fsSL "%PRIMARY_URL%/enlangg.exe" -o "%INSTALL_DIR%\enlangg.exe"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to download enlangg.exe from %PRIMARY_URL%
    exit /b 1
)
curl -fsSL "%PRIMARY_URL%/enlng.exe" -o "%INSTALL_DIR%\enlng.exe"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to download enlng.exe from %PRIMARY_URL%
    exit /b 1
)

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

echo [3/3] Verifying installation:
"%INSTALL_DIR%\enlangg.exe" --version
"%INSTALL_DIR%\enlng.exe" --version

echo =====================================================================
echo   [SUCCESS] Enlangg ^& Enlng installed successfully!
echo =====================================================================
echo You can now run:
echo   enlangg run ^<file.enlng^>
echo   enlng run ^<file.enlng^>
echo   enlangg --help
echo.
echo Documentation ^& Online Playground: https://enlangg.vercel.app
