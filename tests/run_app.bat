@echo off
title Enlang Web App Server
cls
echo ============================================================
echo   ENLANG WEB APP LIVE SERVER
echo   Opening http://localhost:3000 in default browser...
echo ============================================================
start http://localhost:3000
python -m enlg run app.enlgf --p 3000
pause
