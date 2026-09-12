@echo off
setlocal
cd /d "%~dp0"
set OUT=menu.exe

rem ---- locate Visual Studio C++ toolchain automatically ----
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [error] vswhere.exe not found - install Visual Studio 2022 with C++ workload
    exit /b 1
)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (
    echo [error] no Visual Studio with C++ tools found
    exit /b 1
)

call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo [error] vcvars64.bat failed
    exit /b 1
)

set SRC=main.cpp imgui\imgui.cpp imgui\imgui_draw.cpp imgui\imgui_tables.cpp imgui\imgui_widgets.cpp imgui\backends\imgui_impl_dx11.cpp imgui\backends\imgui_impl_win32.cpp
set LIBS=d3d11.lib dxgi.lib d3dcompiler.lib dwmapi.lib windowscodecs.lib ole32.lib

echo [1/2] compiling...
cl /nologo /EHsc /O2 /W3 /utf-8 /std:c++17 /I imgui /I imgui\backends %SRC% /Fe:%OUT% /link /MACHINE:X64 %LIBS% /SUBSYSTEM:WINDOWS
if errorlevel 1 (
    echo [error] build failed
    exit /b 1
)

if not exist 666-logo.png echo [warning] 666-logo.png not found - fallback wordmark will be used

echo [2/2] done: %OUT%
if exist %OUT% (
    echo.
    echo Build OK - run %OUT% to test the UI
)
endlocal
