@echo off
setlocal
set "source=%1"
set "target=%2"
set "source=%source:"=%
set "target=%target:"=%

fc /B "%source%" "%target%">nul
if errorlevel 1 (
  ECHO Copying "%target%" because it is different
  copy "%source%" "%target%"
) else (
  ECHO "%target%" is the same
)
