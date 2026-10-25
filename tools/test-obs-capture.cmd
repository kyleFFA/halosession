@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out mkdir out
set DEP=%LOCALAPPDATA%\HaloPracticeBuild\HCMInternal\x64-windows-static
cl /nologo /std:c++latest /O2 /EHsc /W3 /MT /DUNICODE /D_UNICODE /I"%DEP%\include" /IHCMInternal /IHCMInternal\includes\safetyhook\include /Fo:out\obs_capture_test.obj /Fe:out\obs_capture_test.exe tools\obs_capture_test.cpp /link /LTCG HCMInternal\x64\Release\allocator.obj HCMInternal\x64\Release\easy.obj HCMInternal\x64\Release\inline_hook.obj HCMInternal\x64\Release\mid_hook.obj HCMInternal\x64\Release\os.windows.obj HCMInternal\x64\Release\utility.obj HCMInternal\x64\Release\vmt_hook.obj /LIBPATH:"%DEP%\lib" Zydis.lib Zycore.lib
if errorlevel 1 exit /b 1
out\obs_capture_test.exe
exit /b %errorlevel%
