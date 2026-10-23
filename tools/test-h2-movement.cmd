@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out mkdir out
cl /nologo /std:c++20 /O2 /EHsc /W4 /WX /MT /Fo:out\h2_movement.obj /Fe:out\h2_movement.exe tools\h2_movement_test.cpp
if errorlevel 1 exit /b 1
out\h2_movement.exe
exit /b %errorlevel%
