@Echo Off

Call %~dp0\make.cmd Debug x64 || Goto :EOF
%~dp0\build\bin\Debug\seabolt-test %* || Goto :EOF
