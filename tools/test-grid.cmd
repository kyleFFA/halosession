@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++20 /O2 /EHsc /W4 /WX /MT /Fe:tools\grid_options_test.exe tools\grid_options_test.cpp HCMInternal\WorldLinesScene.cpp HCMInternal\WorldLinesGpu.cpp HCMInternal\WorldLinesEmit.cpp
if errorlevel 1 exit /b 1
tools\grid_options_test.exe
