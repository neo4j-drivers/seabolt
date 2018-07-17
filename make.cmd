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
    Set "VS_TOOLS_ARCH=x64"
    Set "VS_TARGET_PLATFORM=x64"

    If "%~3" == "x86" (
        Set "CMAKE_GENERATOR=Visual Studio 15 2017"
        Set "VS_TOOLS_ARCH=x86"
        Set "VS_TARGET_PLATFORM=Win32"
    )
)

If "%~1" == "MSVC" (
	REM Check if in Visual Studio Developer Command Prompt
	If Not Defined VCINSTALLDIR (
		For %%D In (Enterprise Professional Community BuildTools) Do (
			If Exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\%%D\VC\Auxiliary\Build\vcvarsall.bat" (
				Pushd %cd%
				Call "C:\Program Files (x86)\Microsoft Visual Studio\2017\%%D\VC\Auxiliary\Build\vcvarsall.bat" %VS_TOOLS_ARCH%
				If ERRORLEVEL 0 (
					Popd
					Goto :FoundVS
				)
				Popd
			)
		)
	)

	:FoundVS
	If Not Defined VCINSTALLDIR (
		Echo "Visual Studio 2017 Installation Location could not be determined."
		Echo "Ensure you've it installed and run this script in Visual Studio Developer Command Prompt."
		Goto :Failure
	)
)

If "%~1" == "MINGW" (
    Set "CMAKE_GENERATOR=MinGW Makefiles"
)

Set SEABOLTDIR=%~dp0
Pushd %SEABOLTDIR%

cmake.exe -G "%CMAKE_GENERATOR%" -DCMAKE_BUILD_TYPE=%CMAKE_BUILD% || Goto :Failure

If "%~1" == "MSVC" (
	msbuild.exe seabolt-all.sln /p:Platform=%VS_TARGET_PLATFORM% || Goto :Failure
) Else (
	mingw32-make.exe || Goto :Failure
)

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
