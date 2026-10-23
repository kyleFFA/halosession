@echo off
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist out mkdir out
for %%T in (auto_crouch_identity auto_crouch_target keep_armed) do (
 cl /nologo /std:c++20 /O2 /EHsc /W4 /WX /MT /Fo:out\%%T.obj /Fe:out\%%T.exe tools\%%T_test.cpp
 if errorlevel 1 exit /b 1
 out\%%T.exe
 if errorlevel 1 exit /b 1
)
cl /nologo /std:c++latest /O2 /EHsc /W3 /MT /I"%LOCALAPPDATA%\HaloPracticeBuild\HCMInternal\x64-windows-static\include" /IHCMInternal\includes /IHCMInternal\includes\safetyhook\include /Fo:out\practice_hotkeys.obj /Fe:out\practice_hotkeys.exe tools\practice_hotkeys_test.cpp
if errorlevel 1 exit /b 1
out\practice_hotkeys.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /O2 /EHsc /W4 /WX /MT /Fe:out\grid_options.exe tools\grid_options_test.cpp HCMInternal\WorldLinesScene.cpp HCMInternal\WorldLinesGpu.cpp HCMInternal\WorldLinesEmit.cpp
if errorlevel 1 exit /b 1
out\grid_options.exe
exit /b %errorlevel%
