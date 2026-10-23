@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++20 /O2 /EHsc /W4 /WX /MT /Fe:tools\keep_armed_test.exe tools\keep_armed_test.cpp
if errorlevel 1 exit /b 1
tools\keep_armed_test.exe
