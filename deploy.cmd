@echo off
setlocal
echo ===================================================
echo   Enlangg Official Website - Instant Vercel Deploy
echo ===================================================

set "SRC_DIR=%~dp0website"
set "TMP_DEPLOY=%TEMP%\enlangg_deploy_%RANDOM%"

if not exist "%SRC_DIR%" (
    echo [ERROR] Website directory not found at "%SRC_DIR%"
    exit /b 1
)

echo [1/3] Preparing clean deployment bundle...
if exist "%TMP_DEPLOY%" rmdir /S /Q "%TMP_DEPLOY%" 2>nul
mkdir "%TMP_DEPLOY%"
xcopy /E /Y /I /Q "%SRC_DIR%" "%TMP_DEPLOY%" >nul

echo [2/3] Uploading and deploying to Vercel Production...
pushd "%TMP_DEPLOY%"
call npx --yes vercel --prod --yes
set DEPLOY_EXIT=%ERRORLEVEL%
popd

echo [3/3] Cleaning up temporary workspace...
timeout /t 1 /nobreak >nul
rmdir /S /Q "%TMP_DEPLOY%" 2>nul

if %DEPLOY_EXIT% EQU 0 (
    echo ===================================================
    echo  [SUCCESS] Live at https://enlangg.vercel.app
    echo ===================================================
) else (
    echo ===================================================
    echo  [FAILED] Deployment exited with code %DEPLOY_EXIT%
    echo ===================================================
)

exit /b %DEPLOY_EXIT%
