@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set DEP=%LOCALAPPDATA%\HaloPracticeBuild\HCMInternal\x64-windows-static
set INC=/I"%DEP%\include" /IHCMInternal /IHCMInternal\includes /IHCMInternal\includes\safetyhook\include /IHCMInternal\includes\type_safe\include
cl /nologo /std:c++latest /O2 /EHsc /W3 /MT /Gy /DUNICODE /D_UNICODE %INC% /Fo:out\ /Fe:out\speed_overlay_settings_test.exe tools\speed_overlay_settings_test.cpp HCMInternal\BinarySetting.cpp HCMInternal\UnarySetting.cpp HCMInternal\WaypointList.cpp HCMInternal\ViewAngleLineList.cpp HCMInternal\HCM_imconfig.cpp HCMInternal\SubpixelID.cpp HCMInternal\WindowsUtilities.cpp /link /OPT:REF /LIBPATH:"%DEP%\lib" imgui.lib pugixml.lib user32.lib version.lib
if errorlevel 1 exit /b 1
out\speed_overlay_settings_test.exe
exit /b %errorlevel%
