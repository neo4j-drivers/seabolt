@Echo Off

Call %~dp0\make_debug.cmd || Goto :Exit
%~dp0\build\bin\Debug\seabolt-test.exe %* || Goto :Failure
Exit /b 0

:Failure
	If Not "%TEAMCITY_PROJECT_NAME%" == "" (
		ECHO ##teamcity[buildStatus status='FAILURE' text='test execution failed']
	)

:Exit
	Exit /b 1