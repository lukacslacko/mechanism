@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\start-quest-link.ps1"
if errorlevel 1 pause
