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

If "%~4" == "" (
	Goto :Usage
) Else If Not "%~4" == "package" (
	If Not "%~4" == "install" (
		Goto :Usage
	)
)

If "%~1" == "MSVC" (
    Set "BUILDTYPE=msvc"
    Set "CMAKE_GENERATOR=Visual Studio 15 2017 Win64"

    If "%~3" == "x86" (
        Set "CMAKE_GENERATOR=Visual Studio 15 2017"
    )
)

If "%~1" == "MINGW" (
    Set "BUILDTYPE=mingw"
    Set "CMAKE_GENERATOR=MinGW Makefiles"
)

If "%~2" == "Debug" (
    Set "BUILDCONFIG=debug"
    Set "CMAKE_BUILD=Debug"
)

If "%~2" == "Release" (
    Set "BUILDCONFIG=release"
    Set "CMAKE_BUILD=RelWithDebInfo"
)

Set TARGET=%~4
Set SEABOLTDIR=%~dp0
Set BUILDDIR=%SEABOLTDIR%\build-%BUILDTYPE%-%BUILDCONFIG%
Mkdir %BUILDDIR%
Pushd %BUILDDIR%

cmake.exe -G "%CMAKE_GENERATOR%" -DCMAKE_CONFIGURATION_TYPES=%CMAKE_BUILD% -DCMAKE_BUILD_TYPE=%CMAKE_BUILD% -DCMAKE_INSTALL_PREFIX=dist .. || Goto :Failure
cmake.exe --build . --target %TARGET% --config %CMAKE_BUILD% || Goto :Failure

Popd
Exit /b 0

:Usage
	@Echo "Usage: %~n0 (MSVC|MINGW) (Debug|Release) (x86|x64) (install|package)"
	Goto :Failure
	
:Failure
	If "%NEO4J_CHILD_SCRIPT%" == "" (
		If Not "%TEAMCITY_PROJECT_NAME%" == "" (
			ECHO ##teamcity[buildStatus status='FAILURE' text='compilation failed']
		)
	)
	popd.exe
	Exit /b 1
