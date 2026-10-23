@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out mkdir out
set DEP=%LOCALAPPDATA%\HaloPracticeBuild\HCMInternal\x64-windows-static
set INC=/I"%DEP%\include" /IHCMInternal /IHCMInternal\includes /IHCMInternal\includes\safetyhook\include /IHCMInternal\includes\type_safe\include
rc /nologo /foout\ui-test.res tools\ui-test.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++latest /O2 /EHsc /W3 /MT /DUNICODE /D_UNICODE %INC% /Fo:out\ /Fe:out\sketch_ui_test.exe tools\sketch_ui_test.cpp HCMInternal\SessionSketch.cpp out\ui-test.res /link /LIBPATH:"%DEP%\lib" imgui.lib d3d11.lib dxgi.lib windowscodecs.lib ole32.lib user32.lib
if errorlevel 1 exit /b 1
out\sketch_ui_test.exe
exit /b %errorlevel%
