call %~dp0set_vars.bat
if NOT DEFINED project (
  echo project is not defined
  GOTO:EOF
)
IF NOT DEFINED project_dir (
  echo project_dir is not defined
  GOTO:EOF
)
IF NOT DEFINED runtime_path (
  echo runtime_path is not defined
  GOTO:EOF
)

pushd %project_dir%\bin
@setlocal
set PATH=%runtime_path%
%project%.exe
popd
