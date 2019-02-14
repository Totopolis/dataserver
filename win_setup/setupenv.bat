rem set CMAKE="C:\Program Files (x86)\CMake\bin\cmake.exe"
set CMAKE=cmake.exe

rem set cmake toolchain
rem "to get available cmake toolchains run"
rem %CMAKE% --help

rem choose one of possible cmake toolchain constants:
set VS10_2010="Visual Studio 10 2010 Win64"
set VS11_2012="Visual Studio 11 2012 Win64"
set VS12_2013="Visual Studio 12 2013 Win64"
set VS14_2015="Visual Studio 14 2015 Win64"
set VS15_2017="Visual Studio 15 2017 Win64"

set CMAKETOOLCHAIN=%VS15_2017%
rem set CMAKETOOLCHAIN=%VS14_2015%
set TOOLCHAINPATH=%CMAKETOOLCHAIN: =_%

rem choose build type from one of possible constants
rem set BUILD_TYPE_DEBUG=Debug
rem set BUILD_TYPE_RELEASE=Release
rem set BUILD_TYPE_RELWITHDEBINFO=RelWithDebInfo
rem set BUILD_TYPE_MINSIZEREL=MinSizeRel

if %DEPLOY_BUILD_TYPE%.==. GOTO No1
set BUILD_TYPE=%DEPLOY_BUILD_TYPE%
GOTO No2
:No1
set BUILD_TYPE=Release
rem set BUILD_TYPE=Debug
:No2

set CMAKEGENERATOR=-G %CMAKETOOLCHAIN%
set TOOLCHAINTAG=%TOOLCHAINPATH:~1,-1%_%BUILD_TYPE%
set SCRIPTS_DIR=%CD%
set PROJECT_ROOT_DIR=%SCRIPTS_DIR%\..
set BUILD_DIR=%PROJECT_ROOT_DIR%\build
set DEPLOY_DIR=%PROJECT_ROOT_DIR%\install\%BUILD_TYPE%

rem setup msvs environment to build openssl
call setup_msvs_env.bat