set project=storecast
set "build_base_dir=D:\coding\build"
rem set "lib_dir=D:\coding\lib"

set bat_path=%~dp0
set project_dir=%bat_path:~0, -1%
if "%build_base_dir%" == "" (
  set "build_base_dir=%project_dir%"
)

set "build_dir=%build_base_dir%\%project%"
if not exist "%build_dir%" (
  mkdir "%build_dir%"
)
if not exist "%project_dir%\build" (
  mklink /J "%project_dir%\build" "%build_dir%"
)

rem Library dirs
rem set "qt_dir=%lib_dir%\Qt5.8.0\5.8\msvc2015_64"
rem set "qt_dir=%lib_dir%\Qt5.9.0beta3\5.9\msvc2015_64"

if "%1"=="build" (
  REM call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" amd64
  call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
  echo on
)

REM Add runtime library paths here
set runtime_path=%project_dir%\bin
