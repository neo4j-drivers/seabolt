@Echo Off

%~dp0\make.cmd Release x64 || Exit /B %ERRORLEVEL%
