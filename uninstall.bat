@echo off
setlocal

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: Please run as Administrator!
    pause
    exit /b 1
)

echo.
echo ==================================================
echo  Unified Switch – Uninstaller
echo ==================================================
echo.

echo Removing startup task...
schtasks /delete /tn "Unified Switch" /f >nul 2>&1
if %errorLevel% equ 0 (
    echo   Task removed successfully.
) else (
    echo   Task not found or already removed.
)

set "FOLDER=%~dp0.."
echo Removing UnifiedSwitch folder...
rmdir /s /q "%FOLDER%" 2>nul

if exist "%FOLDER%" (
    echo   Folder deletion failed (files may be in use).
    echo   Please manually delete: %FOLDER%
    echo.
    echo Uninstall completed with warnings.
    pause
    exit /b 2
) else (
    echo   Folder removed successfully.
)

echo.
echo Unified Switch has been completely uninstalled.
echo.
pause
exit /b 0