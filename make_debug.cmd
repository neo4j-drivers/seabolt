@Echo Off

Call %~dp0\make.cmd Debug x64 || Goto :Failure
Exit /b 0

:Failure
Exit /b 1
