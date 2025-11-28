@echo off
setlocal

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: Please run as Administrator!
    pause
    exit /b 1
)

set "EXE_PATH=%~dp0UnifiedSwitch.exe"
if not exist "%EXE_PATH%" (
    echo ERROR: %~nx0 not found!
    pause
    exit /b 1
)

schtasks /delete /tn "Unified Switch" /f >nul 2>&1
schtasks /create /tn "Unified Switch" /tr "\"%EXE_PATH%\"" /sc onlogon /rl highest /f /delay 0000:10 >nul

if %errorLevel% equ 0 (
    echo Installation successful!
) else (
    echo ERROR: Task creation failed.
)

pause