@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set H2INC=/I"%LOCALAPPDATA%\HaloPracticeBuild\HCMInternal\x64-windows-static\include" /IHCMInternal /IHCMInternal\includes /IHCMInternal\includes\safetyhook\include /IHCMInternal\includes\type_safe\include
cl /nologo /std:c++latest /O2 /EHsc /W3 /MT %H2INC% /Fo:out\h2_transition.obj /Fe:out\h2_transition.exe tools\h2_transition_test.cpp
if errorlevel 1 exit /b 1
out\h2_transition.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++latest /O2 /EHsc /W3 /MT %H2INC% /Fo:out\ /Fe:out\h2_renderer.exe tools\h2_renderer_test.cpp HCMInternal\H2CV_Renderer.cpp
if errorlevel 1 exit /b 1
out\h2_renderer.exe
exit /b %errorlevel%
