@echo off
setlocal ENABLEDELAYEDEXPANSION

REM ----------------------------
REM Usage check
REM ----------------------------
if "%~1"=="" (
    echo Usage:
    echo   read_T3_output.bat ^<IP^> [ObjectIndex]
    echo.
    echo Examples:
    echo   read_T3_output.bat 192.168.1.14 60
    echo   read_T3_output.bat 192.168.1.14
    exit /b 1
)

set IP=%1
set COMMUNITY=public

REM ----------------------------
REM OID base definitions
REM ----------------------------
set OID_INDEX=1.3.6.1.4.1.64991.1.2.0
set OID_CFGTYPE=1.3.6.1.4.1.64991.1.2.1
set OID_ANALOG=1.3.6.1.4.1.64991.1.2.2
set OID_BINARY=1.3.6.1.4.1.64991.1.2.3
set OID_DESC=1.3.6.1.4.1.64991.1.2.4
set OID_UNIT=1.3.6.1.4.1.64991.1.2.5

REM ----------------------------
REM Decide range
REM ----------------------------
if "%~2"=="" (
    set START=0
    set END=63
) else (
    set START=%2
    set END=%2
)

REM ----------------------------
REM Main loop
REM ----------------------------
for /L %%i in (%START%,1,%END%) do (
    echo.
    echo ===============================
    echo Testing Object %%i
    echo ===============================

	snmpget -v2c -L n -c %COMMUNITY% %IP% %OID_INDEX%.%%i %OID_CFGTYPE%.%%i %OID_ANALOG%.%%i %OID_BINARY%.%%i %OID_DESC%.%%i %OID_UNIT%.%%i
)

echo.
echo Done.
endlocal
