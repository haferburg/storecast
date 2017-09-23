call %~dp0set_vars.bat build
if NOT DEFINED project (
  echo project is not defined
  GOTO:EOF
)
IF NOT DEFINED project_dir (
  echo project_dir is not defined
  GOTO:EOF
)
if exist %project_dir%\bin\%project%.sln (
  devenv %project_dir%\bin\%project%.sln
) else (
  devenv /debugexe %project_dir%\bin\%project%.exe
)
