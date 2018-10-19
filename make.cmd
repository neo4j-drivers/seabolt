@Echo Off

If "%~1" == "" (
    Goto :Usage
) Else If Not "%~1" == "MSVC" (
  	If Not "%~1" == "MINGW" (
  		Goto :Usage
  	)
)

If "%~2" == "" (
	Goto :Usage
) Else If Not "%~2" == "Debug" (
	If Not "%~2" == "Release" (
		Goto :Usage
	)
)

If "%~3" == "" (
	Goto :Usage
) Else If Not "%~3" == "x86" (
	If Not "%~3" == "x64" (
		Goto :Usage
	)
)

Set "CMAKE_BUILD=%~2"

If "%~1" == "MSVC" (
    Set "CMAKE_GENERATOR=Visual Studio 15 2017 Win64"

    If "%~3" == "x86" (
        Set "CMAKE_GENERATOR=Visual Studio 15 2017"
    )
)

If "%~1" == "MINGW" (
    Set "CMAKE_GENERATOR=MinGW Makefiles"
)

Set SEABOLTDIR=%~dp0
Set BUILDDIR=%SEABOLTDIR%\build
Mkdir %BUILDDIR%
Pushd %BUILDDIR%

cmake.exe -G "%CMAKE_GENERATOR%" -DCMAKE_BUILD_TYPE=%CMAKE_BUILD% -DCMAKE_INSTALL_PREFIX=dist .. || Goto :Failure
cmake.exe --build . --target install || Goto :Failure

Popd
Exit /b 0

:Usage
	@Echo "Usage: %~n0 (MSVC|MINGW) (Debug|Release) (x86|x64)"
	Goto :Failure
	
:Failure
	If "%NEO4J_CHILD_SCRIPT%" == "" (
		If Not "%TEAMCITY_PROJECT_NAME%" == "" (
			ECHO ##teamcity[buildStatus status='FAILURE' text='compilation failed']
		)
	)
	popd.exe
	Exit /b 1
