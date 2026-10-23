@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out\message-tests mkdir out\message-tests
set DEP=%LOCALAPPDATA%\HaloPracticeBuild\HCMInternal\x64-windows-static
set INC=/I"%DEP%\include" /IHCMInternal /IHCMInternal\includes /IHCMInternal\includes\safetyhook\include /IHCMInternal\includes\type_safe\include
cl /nologo /std:c++latest /O2 /EHsc /W3 /MT /DUNICODE /D_UNICODE %INC% /c /Fo:out\message-tests\ tools\message_render_test.cpp HCMInternal\MessagesGUI.cpp HCMInternal\RenderTextHelper.cpp
if errorlevel 1 exit /b 1
set OBJ=HCMInternal\x64\Release
link /nologo /LTCG /OUT:out\message_render_test.exe out\message-tests\*.obj out\transition-tests\ModuleHook.obj out\transition-tests\ModuleHookManager.obj out\transition-tests\ModuleCache.obj out\transition-tests\MultilevelPointer.obj out\transition-tests\WindowsUtilities.obj %OBJ%\pch.obj %OBJ%\BinarySetting.obj %OBJ%\UnarySetting.obj %OBJ%\WaypointList.obj %OBJ%\ViewAngleLineList.obj %OBJ%\SubpixelID.obj %OBJ%\SettingsEnum.obj %OBJ%\HCM_imconfig.obj %OBJ%\ScopedImFontScaler.obj %OBJ%\allocator.obj %OBJ%\easy.obj %OBJ%\inline_hook.obj %OBJ%\mid_hook.obj %OBJ%\os.windows.obj %OBJ%\utility.obj %OBJ%\vmt_hook.obj /LIBPATH:"%DEP%\lib" Zydis.lib Zycore.lib imgui.lib pugixml.lib version.lib psapi.lib user32.lib shell32.lib advapi32.lib
if errorlevel 1 exit /b 1
python -c "import subprocess; subprocess.run(['out/message_render_test.exe'],check=True,timeout=30)"
exit /b %errorlevel%
