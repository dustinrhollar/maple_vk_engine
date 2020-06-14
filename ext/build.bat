@echo off
setlocal EnableDelayedExpansion

SET HOST_DIR=%~dp0
SET HOST_DIR=%HOST_DIR:~0,-1%

pushd %HOST_DIR%\mat_gen
	call build.bat
	xcopy build\mat_gen.exe ..\..\build\
popd