@echo off
setlocal
call %~dp0set_vars.bat build
@echo off

if NOT DEFINED project (
  echo project is not defined
  GOTO:EOF
)
IF NOT DEFINED project_dir (
  echo project_dir is not defined
  GOTO:EOF
)
IF NOT DEFINED build_dir (
  echo build_dir is not defined
  GOTO:EOF
)

pushd %build_dir%

REM /Zi Generates complete debugging information
REM /FC Display full path of source code files passed to cl.exe in diagnostic text.
REM /Fe Renames the executable file
REM /Fm Creates a mapfile
REM /MP[n] use up to 'n' processes for compilation
REM /Gm[-] enable minimal rebuild
REM /GR[-] enable C++ RTTI
REM /GL[-] enable link-time code generation
REM /GA optimize for Windows Application
REM warning C4127: conditional expression is constant
REM warning C4714: function ... marked as __forceinline not inlined (because
REM                QString::trimmed)
REM /c Compile only, no link

set COMPILER_FLAGS_DEBUG=/MTd
set COMPILER_FLAGS_RELEASE=/MT

if "%DEBUG%"=="debug" (
  set COMPILER_FLAGS=%COMPILER_FLAGS_DEBUG%
) else (
  set COMPILER_FLAGS=%COMPILER_FLAGS_RELEASE%
)

set COMPILER_FLAGS=/nologo^
 /Gm /EHsc^
 /D_CRT_NONSTDC_NO_WARNINGS^
 /I "%project_dir%\src"^
 %COMPILER_FLAGS% /Zi /WX /W4 /wd4201 /wd4100 /wd4127 /wd4996 /FC /GR- /GL-

set LINKER_FLAGS_DEBUG=
set LINKER_FLAGS_RELEASE=
if "%DEBUG%"=="debug" (
  set LINKER_FLAGS=%LINKER_FLAGS_DEBUG%
) else (
  set LINKER_FLAGS=%LINKER_FLAGS_RELEASE%
)
set LINKER_FLAGS= /subsystem:console^
 /INCREMENTAL^
 user32.lib^
 %LINKER_FLAGS%

rem Taskkill /IM %project%.exe >nul 2>&1

ECHO ------- %project%.exe ---------------------------------------------------------
cl.exe %COMPILER_FLAGS%^
 /Fm%project%.map /Fe%project%.exe^
 %project_dir%\src\main.cpp^
 /link %LINKER_FLAGS%

popd

if NOT EXIST bin ( mkdir bin )
call copy_if_different %build_dir%\%project%.exe bin\%project%.exe

echo Running tests
call run.bat
