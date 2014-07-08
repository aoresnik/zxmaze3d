@echo off

rem This script relies on make from cygwin
set CYGWIN_DIR=E:\cygwin

rem Sets PATH, ZCCCFG and Z80_OZFILES
call E:\Programiranje\Platforme\ZX-Spectrum\z88dk\setenv.bat

if not %1 == "" goto :run
echo Compiles and optionally runs the specified C program with z88dk
echo syntax: _name_.c [/start]
goto skip_run

:run
echo C file name: %1
for %%i in ("%1") do set FN_BASE=%%~ni
echo Base name  : %FN_BASE%

rem build with make
%CYGWIN_DIR%\bin\make %FN_BASE%.tap

if errorlevel 1 goto skip_run

echo Compile OK

if not "%2" == "/start" goto skip_run

echo Running
start %FN_BASE%.tap
rem e:\Emulatorji\RealSpectrum\realspec D:\Programiranje\zxmaze3d\%FN_BASE%.tap

:skip_run
