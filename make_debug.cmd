@Echo Off

%~dp0\make.cmd Debug x64 || Exit /B %ERRORLEVEL%
