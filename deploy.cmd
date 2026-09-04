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

echo [1/2] Navigating to website directory...
pushd "%SRC_DIR%"

echo [2/2] Uploading and deploying to Vercel Production...
call npx --yes vercel --prod --yes
set DEPLOY_EXIT=%ERRORLEVEL%
popd

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
