@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out mkdir out
cl /nologo /std:c++20 /O2 /EHsc /W4 /WX /MT /Fo:out\quick_menu.obj /Fe:out\quick_menu.exe tools\quick_menu_test.cpp
if errorlevel 1 exit /b 1
out\quick_menu.exe
exit /b %errorlevel%
