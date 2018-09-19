
if defined OLDTOOLCHAIN  (
	echo  "%OLDTOOLCHAIN%"
	echo  "%CMAKETOOLCHAIN%"

	if "%CMAKETOOLCHAIN:~1,-1%" ==  "%OLDTOOLCHAIN:~1,-1%" (
		echo "skipping setup"
		goto SKIP_SETUP
	)
)

rem setup visual studio native tools command prompt(to built openssl)

if %CMAKETOOLCHAIN%==%VS11_2012% (
	goto VS11_2012_LABEL
)

if %CMAKETOOLCHAIN%==%VS12_2013% (
	goto VS12_2013_LABEL
)

if %CMAKETOOLCHAIN%==%VS14_2015% (
	goto VS14_2015_LABEL
)

if %CMAKETOOLCHAIN%==%VS15_2017% (
	goto VS15_2017_LABEL
)

:VS11_2012_LABEL
set OLDTOOLCHAIN=%CMAKETOOLCHAIN%
echo "defined VS11_2012"

call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x86_amd64

goto SKIP_SETUP

:VS12_2013_LABEL
set OLDTOOLCHAIN=%CMAKETOOLCHAIN%
echo "defined VS12_2013"

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64

goto SKIP_SETUP

:VS14_2015_LABEL
set OLDTOOLCHAIN=%CMAKETOOLCHAIN%
echo "defined VS14_2015"

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64

goto SKIP_SETUP

:VS15_2017_LABEL
set OLDTOOLCHAIN=%CMAKETOOLCHAIN%
echo "defined VS15_2017"

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64

goto SKIP_SETUP

:SKIP_SETUP