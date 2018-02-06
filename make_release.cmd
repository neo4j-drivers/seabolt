@Echo Off

Call %~dp0\make.cmd Release x64 || Goto :EOF
Exit /b 0
