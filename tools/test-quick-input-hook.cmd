@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out\quick-hook mkdir out\quick-hook
ml64 /nologo /c /Foout\quick-hook\fixture.obj tools\quick_input_fixture.asm
if errorlevel 1 exit /b 1
set DEP=%LOCALAPPDATA%\HaloPracticeBuild\HCMInternal\x64-windows-static
set INC=/I"%DEP%\include" /IHCMInternal /IHCMInternal\includes /IHCMInternal\includes\safetyhook\include /IHCMInternal\includes\type_safe\include
cl /nologo /std:c++latest /O2 /EHsc /W3 /MT /DUNICODE /D_UNICODE %INC% /c /Fo:out\quick-hook\ tools\quick_input_hook_test.cpp
if errorlevel 1 exit /b 1
set OBJ=HCMInternal\x64\Release
link /nologo /LTCG /OUT:out\quick_input_hook_test.exe out\quick-hook\*.obj %OBJ%\allocator.obj %OBJ%\easy.obj %OBJ%\inline_hook.obj %OBJ%\mid_hook.obj %OBJ%\os.windows.obj %OBJ%\utility.obj %OBJ%\vmt_hook.obj /LIBPATH:"%DEP%\lib" Zydis.lib Zycore.lib imgui.lib version.lib psapi.lib user32.lib shell32.lib advapi32.lib
if errorlevel 1 exit /b 1
python -c "import subprocess; subprocess.run(['out/quick_input_hook_test.exe'],check=True,timeout=30)"
exit /b %errorlevel%
