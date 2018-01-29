@Echo Off

SetLocal

Call %~dp0\make.cmd Debug x64 || Exit /B %ERRORLEVEL%
%~dp0\build\bin\Debug\seabolt-test %* || Exit /B %ERRORLEVEL%
