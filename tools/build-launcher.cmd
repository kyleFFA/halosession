@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out mkdir out
rc /nologo /fo out\Session.res SessionLauncher\Session.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /O2 /EHsc /W4 /WX /MT /DUNICODE /D_UNICODE /Zi /Fo:out\launcher.obj /Fd:out\launcher-compile.pdb /Fe:out\Session.exe SessionLauncher\main.cpp out\Session.res user32.lib version.lib gdi32.lib /link /SUBSYSTEM:WINDOWS /DEBUG /PDB:out\Session.pdb /MANIFESTUAC:"level='requireAdministrator' uiAccess='false'"
exit /b %errorlevel%
