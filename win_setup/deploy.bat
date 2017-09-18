if %1.==. GOTO No1
set DEPLOY_BUILD_TYPE=%1
GOTO No2
:No1
set DEPLOY_BUILD_TYPE=Release
:No2

call setupenv.bat
mkdir %BUILD_DIR%

rem force reinstall every time by recreating deploy directory
del /S /Q %DEPLOY_DIR%
rmdir /s /q %DEPLOY_DIR%

mkdir %DEPLOY_DIR_BASE%
mkdir %DEPLOY_DIR%

cd %SCRIPTS_DIR%
call dataserver_build.bat

