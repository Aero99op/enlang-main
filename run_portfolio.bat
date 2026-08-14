@echo off
title Enlang 3D Portfolio Server
cls
echo ============================================================
echo   ENLANG 3D PORTFOLIO LIVE SERVER
echo   Opening http://localhost:2222 in default browser...
echo ============================================================
start http://localhost:2222
python -m enlg run portfolio.enlgf --p 2222
pause
