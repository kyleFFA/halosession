@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out mkdir out
cl /nologo /std:c++20 /O2 /EHsc /W3 /MT /Fo:out\ /Fe:out\obs_grid_render_test.exe tools\obs_grid_render_test.cpp HCMInternal\WorldLinesGpu.cpp /link d3d11.lib
if errorlevel 1 exit /b 1
out\obs_grid_render_test.exe
exit /b %errorlevel%
