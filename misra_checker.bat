@echo off

REM === Horodatage ===
set DATESTR=%DATE:~6,4%%DATE:~3,2%%DATE:~0,2%
set TIMESTR=%TIME:~0,2%%TIME:~3,2%%TIME:~6,2%
set TIMESTR=%TIMESTR: =0%

REM === Dossier de sortie ===
set REPORT_DIR=misra_report

REM === Crée le dossier s'il n'existe pas ===
if not exist "%REPORT_DIR%" (
    mkdir "%REPORT_DIR%"
)

REM === Lancement cppcheck ===
cppcheck ^
 --enable=all ^
 --addon=misra ^
 --std=c11 ^
 --inline-suppr ^
 --force ^
 -I App/Inc ^
 App/Src/cli_uart.c ^
 2> "%REPORT_DIR%\misra_report_%DATESTR%_%TIMESTR%.txt"
