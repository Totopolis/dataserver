call setupenv.bat

cd %BUILD_DIR%
mkdir dataserver_build
cd dataserver_build
mkdir %TOOLCHAINTAG%
cd %TOOLCHAINTAG%

%CMAKE% %CMAKEGENERATOR% -DCMAKE_INSTALL_PREFIX=%DEPLOY_DIR% -DDEPLOY_DIR=%DEPLOY_DIR% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% %PROJECT_ROOT_DIR%
%CMAKE% --build . --target install --config %BUILD_TYPE%
cd %SCRIPTS_DIR%