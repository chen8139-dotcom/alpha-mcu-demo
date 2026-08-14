@echo off
setlocal EnableExtensions
REM Run from Keil with current directory = folder containing this .uvprojx (MDK-ARM).
set "HERE=%~dp0"
set "FROMELF=D:\software\MDK530\ARM\ARMCC\bin\fromelf.exe"
set "PSH=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"

if not exist "%HERE%bin" mkdir "%HERE%bin"
if not exist "%FROMELF%" (
  echo after_build: fromelf not found: "%FROMELF%" 1>&2
  echo FROMELF_MISSING> "%HERE%bin\Project.chk.err"
  exit /b 1
)

"%FROMELF%" --bin "%HERE%Objects\Project.axf" --output "%HERE%bin\Project.bin"
if errorlevel 1 (
  echo after_build: fromelf failed 1>&2
  echo FROMELF_FAILED> "%HERE%bin\Project.chk.err"
  exit /b 1
)

if not exist "%HERE%bin\Project.bin" (
  echo after_build: Project.bin missing after fromelf 1>&2
  echo NO_PROJECT_BIN> "%HERE%bin\Project.chk.err"
  exit /b 1
)

if not exist "%PSH%" (
  echo after_build: powershell not found: "%PSH%" 1>&2
  echo POWERSHELL_MISSING> "%HERE%bin\Project.chk.err"
  exit /b 1
)

"%PSH%" -NoProfile -ExecutionPolicy Bypass -File "%HERE%tools\gen_bin_sum16.ps1"
if errorlevel 1 (
  echo after_build: gen_bin_sum16.ps1 failed 1>&2
  echo PS1_FAILED> "%HERE%bin\Project.chk.err"
  exit /b 1
)

if not exist "%HERE%bin\Project.chk" (
  echo after_build: Project.chk not created 1>&2
  echo NO_CHK> "%HERE%bin\Project.chk.err"
  exit /b 1
)

echo BIN_SUM16:
type "%HERE%bin\Project.chk"
exit /b 0
