@echo off
chcp 65001 >nul
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Apply-MinotaurUpdate.ps1" %*
set "minotaur_update_exit_code=%errorlevel%"
echo.
pause
exit /b %minotaur_update_exit_code%
