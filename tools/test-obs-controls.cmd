@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out mkdir out
set DEP=%LOCALAPPDATA%\HaloPracticeBuild\HCMInternal\x64-windows-static
cl /nologo /std:c++latest /O2 /EHsc /W3 /MT /DUNICODE /D_UNICODE /I"%DEP%\include" /IHCMInternal /Fo:out\obs_controls_test.obj /Fe:out\obs_controls_test.exe tools\obs_controls_test.cpp /link /LIBPATH:"%DEP%\lib" imgui.lib user32.lib
if errorlevel 1 exit /b 1
out\obs_controls_test.exe
exit /b %errorlevel%
