@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out mkdir out
set DEP=%LOCALAPPDATA%\HaloPracticeBuild\HCMInternal\x64-windows-static
cl /nologo /std:c++latest /O2 /EHsc /W3 /MT /I"%DEP%\include" /Fo:out\obs_h3_grid_route_test.obj /Fe:out\obs_h3_grid_route_test.exe tools\obs_h3_grid_route_test.cpp
if errorlevel 1 exit /b 1
out\obs_h3_grid_route_test.exe
exit /b %errorlevel%
