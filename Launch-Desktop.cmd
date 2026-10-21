@echo off
setlocal
set "MECHANISM_EXE=%~dp0Saved\StagedBuilds\Windows\MaterialHall.exe"
if not exist "%MECHANISM_EXE%" (
    echo Packaged app missing. Run Tools\build-windows.ps1 -Action Package first.
    pause
    exit /b 1
)
start "" /D "%~dp0Saved\StagedBuilds\Windows" "%MECHANISM_EXE%" -nohmd -windowed -ResX=1600 -ResY=1000
