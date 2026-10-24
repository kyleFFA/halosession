@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out mkdir out
set DEP=%LOCALAPPDATA%\HaloPracticeBuild\HCMInternal\x64-windows-static
cl /nologo /std:c++latest /O2 /EHsc /W4 /WX /MT /I"%DEP%\include" /IHCMInternal /Fo:out\workshop_input_test.obj /Fe:out\workshop_input_test.exe tools\workshop_input_test.cpp
if errorlevel 1 exit /b 1
out\workshop_input_test.exe
exit /b %errorlevel%
