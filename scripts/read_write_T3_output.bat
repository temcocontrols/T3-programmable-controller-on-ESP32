@echo off
setlocal

REM ----------------------------
REM Usage check
REM ----------------------------
if "%~2"=="" (
    echo Usage:
    echo   read_write_test.bat ^<IP^> ^<index^>
    echo.
    echo Example:
    echo   read_write_test.bat 192.168.1.15 0
    exit /b 1
)

set IP=%1
set INDEX=%2

REM ----------------------------
REM SNMP parameters
REM ----------------------------
set VER=-v2c
set LOG=-L n

set RO_COMM=public
set RW_COMM=private

REM ----------------------------
REM OID base
REM ----------------------------
set OID_INT_BASE=1.3.6.1.4.1.2026.1.2.3
set OID_STR_BASE=1.3.6.1.4.1.2026.1.2.4

set OID_INT=%OID_INT_BASE%.%INDEX%
set OID_STR=%OID_STR_BASE%.%INDEX%

REM ----------------------------
REM INTEGER TEST
REM ----------------------------
echo.
echo ===============================
echo INTEGER OID TEST (Index %INDEX%)
echo ===============================

echo [GET - before SET]
snmpget %VER% %LOG% -c %RO_COMM% %IP% %OID_INT%

echo [SET - INTEGER = 30]
snmpset %VER% %LOG% -c %RW_COMM% %IP% %OID_INT% i 30

echo [GET - after SET]
snmpget %VER% %LOG% -c %RO_COMM% %IP% %OID_INT%

REM ----------------------------
REM STRING TEST
REM ----------------------------
echo.
echo ===============================
echo STRING OID TEST (Index %INDEX%)
echo ===============================

echo [GET - before SET]
snmpget %VER% %LOG% -c %RW_COMM% %IP% %OID_STR%

echo [SET - STRING = OP_T1]
snmpset %VER% %LOG% -c %RW_COMM% %IP% %OID_STR% s OP_T1

echo [GET - after SET]
snmpget %VER% %LOG% -c %RW_COMM% %IP% %OID_STR%

echo.
echo Test completed for index %INDEX%.
endlocal
