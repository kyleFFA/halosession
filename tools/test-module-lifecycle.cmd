@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out\transition-fixtures mkdir out\transition-fixtures
if not exist out\transition-tests mkdir out\transition-tests
cl /nologo /std:c++20 /Od /EHsc /MT /LD /Fo:out\transition-fixtures\fixture.obj /Fe:out\transition-fixtures\halo2.dll tools\transition_fixture.cpp
if errorlevel 1 exit /b 1
for %%G in (halo3 halo3odst haloreach) do copy /y out\transition-fixtures\halo2.dll out\transition-fixtures\%%G.dll >nul
set DEP=%LOCALAPPDATA%\HaloPracticeBuild\HCMInternal\x64-windows-static
set INC=/I"%DEP%\include" /IHCMInternal /IHCMInternal\includes /IHCMInternal\includes\safetyhook\include /IHCMInternal\includes\type_safe\include
cl /nologo /std:c++latest /O2 /EHsc /W3 /MT /DUNICODE /D_UNICODE %INC% /c /Fo:out\transition-tests\ tools\module_lifecycle_test.cpp HCMInternal\ModuleHook.cpp HCMInternal\ModuleHookManager.cpp HCMInternal\ModuleCache.cpp HCMInternal\MultilevelPointer.cpp HCMInternal\WindowsUtilities.cpp
if errorlevel 1 exit /b 1
set OBJ=HCMInternal\x64\Release
link /nologo /LTCG /OUT:out\module_lifecycle_test.exe out\transition-tests\*.obj %OBJ%\allocator.obj %OBJ%\easy.obj %OBJ%\inline_hook.obj %OBJ%\mid_hook.obj %OBJ%\os.windows.obj %OBJ%\utility.obj %OBJ%\vmt_hook.obj /LIBPATH:"%DEP%\lib" Zydis.lib Zycore.lib imgui.lib version.lib psapi.lib user32.lib shell32.lib advapi32.lib
if errorlevel 1 exit /b 1
python -c "import subprocess; subprocess.run(['out/module_lifecycle_test.exe'],check=True,timeout=45)"
exit /b %errorlevel%
