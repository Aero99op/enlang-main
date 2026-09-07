@echo off
setlocal enabledelayedexpansion

echo =====================================================================
echo     ENLANGG SOVEREIGN 7-IN-1 SUITE - Command Prompt (CMD) Installer
echo =====================================================================

set "INSTALL_DIR=%USERPROFILE%\.enlangg\bin"
if not exist "%INSTALL_DIR%" (
    mkdir "%INSTALL_DIR%"
)

set "PRIMARY_URL=https://enlangg.vercel.app"

echo [1/3] Installing 7 Sovereign Executables to: %INSTALL_DIR%
echo   - enlangg.exe (Universal Toolchain CLI)
echo   - enlng.exe   (Core Language Compiler)
echo   - enlngdb.exe (Pure C Database Engine)
echo   - enlngf.exe  (Frontend Markup & Web Studio)
echo   - enlngd.exe  (Design Tokens & Stylesheet Engine)
echo   - enlngs.exe  (Reactive Fullstack Script Engine)
echo   - enlngm.exe  (Mobile Native & HAL Compiler)

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

echo   Downloading sovereign production binaries...
for %%B in (enlangg.exe enlng.exe enlngdb.exe enlngf.exe enlngd.exe enlngs.exe enlngm.exe) do (
    curl -fsSL "%PRIMARY_URL%/%%B" -o "%INSTALL_DIR%\%%B"
    if !errorlevel! neq 0 (
        echo [WARNING] Could not fetch %%B from remote, checking local fallback...
        if exist "%~dp0%%B" copy /y "%~dp0%%B" "%INSTALL_DIR%\%%B" >nul
    )
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
echo Available Commands:
echo   enlangg <file>                    (Universal Toolchain)
echo   enlng run <file.enlng>            (Core Backend Logic)
echo   enlngdb <script.enlngdb>          (Pure C Microsecond Database)
echo   enlngf <app.enlngf> --port 3000   (Frontend & Web Studio)
echo   enlngd <theme.enlngd> -o out.css  (Design Tokens to CSS)
echo   enlngs <logic.enlngs> -o out.js   (Reactive Fullstack Scripts)
echo   enlngm <app.enlngm>               (Mobile Native & HAL)
echo.
echo Documentation & Online Playground: https://enlangg.vercel.app
