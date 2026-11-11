@echo off
rem Builds bin\autobreak.dll and bin\autoinject.exe (needs Visual Studio with the C++ x64 tools).
set VCVARS=%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat
if not exist "%VCVARS%" set VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
call "%VCVARS%" >nul
cd /d %~dp0
if not exist bin mkdir bin
cl /nologo /O2 /EHsc /LD src\autobreak.cpp user32.lib /Fo:bin\ /Fe:bin\autobreak.dll || exit /b 1
cl /nologo /O2 /EHsc src\autoinject.cpp user32.lib gdi32.lib /Fo:bin\ /Fe:bin\autoinject.exe || exit /b 1
