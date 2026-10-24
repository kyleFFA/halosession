@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out mkdir out
set DEP=%LOCALAPPDATA%\HaloPracticeBuild\HCMInternal\x64-windows-static
cl /nologo /std:c++latest /O2 /EHsc /W4 /WX /MT /I"%DEP%\include" /IHCMInternal /Fo:out\menu_spot_test.obj /Fe:out\menu_spot_test.exe tools\menu_spot_test.cpp
if errorlevel 1 exit /b 1
out\menu_spot_test.exe
exit /b %errorlevel%
