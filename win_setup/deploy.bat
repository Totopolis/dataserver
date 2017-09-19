if %1.==. GOTO No1
set DEPLOY_BUILD_TYPE=%1
GOTO No2
:No1
set DEPLOY_BUILD_TYPE=Release
:No2

call setupenv.bat
mkdir %BUILD_DIR%

cd %SCRIPTS_DIR%
call dataserver_build.bat

