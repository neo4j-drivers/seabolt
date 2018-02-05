@Echo Off

If "%~1" == "" (
	Goto :Usage
) Else If Not "%~1" == "Debug" (
	If Not "%~1" == "Release" (
		Goto :Usage
	)
)

If "%~2" == "" (
	Goto :Usage
) Else If Not "%~2" == "x86" (
	If Not "%~2" == "x64" (
		Goto :Usage
	)
)

Set "CMAKE_BUILD=%~1"
Set "CMAKE_GENERATOR=Visual Studio 15 2017 Win64"
Set "VS_TOOLS_ARCH=x64"
Set "VS_TARGET_PLATFORM=x64"

If "%~2" == "x86" (
	Set "CMAKE_GENERATOR=Visual Studio 15 2017"
	Set "VS_TOOLS_ARCH=x86"
	Set "VS_TARGET_PLATFORM=Win32"
)

REM Check if in Visual Studio Developer Command Prompt
If Not Defined VCINSTALLDIR (
	For %%D In (Enterprise Professional Community BuildTools) Do (
		If Exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\%%D\VC\Auxiliary\Build\vcvarsall.bat" (
			Call "C:\Program Files (x86)\Microsoft Visual Studio\2017\%%D\VC\Auxiliary\Build\vcvarsall.bat" %VS_TOOLS_ARCH%
			If ERRORLEVEL 0 (
				Goto :FoundVS
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

Set SEABOLTDIR=%~dp0
pushd.exe %SEABOLTDIR%

cmake.exe -G "%CMAKE_GENERATOR%" -DCMAKE_BUILD_TYPE=%CMAKE_BUILD% || Goto :Failure
msbuild.exe seabolt-all.sln /p:Platform=%VS_TARGET_PLATFORM% || Goto :Failure

popd.exe
Exit /b 0

:Usage
	@Echo "Usage: %~n0 (Debug|Release) (x86|x64)"
	Goto :Failure
	
:Failure
	If Not "%TEAMCITY_PROJECT_NAME%" == "" (
		ECHO ##teamcity[buildStatus status='FAILURE' text='compilation failed']
	)
	popd.exe
	Goto :EOF
