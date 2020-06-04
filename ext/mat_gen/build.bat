@echo off
setlocal EnableDelayedExpansion

:: Project directories
SET HOST_DIR=%~dp0
SET HOST_DIR=%HOST_DIR:~0,-1%
SET INC_DIR=
SET EXT_DIR=%HOST_DIR%\ext
SET SRC_DIR=%HOST_DIR%\ext\mat_gen
SET BUILD_DIR=%HOST_DIR%\build

:: unity Build files
SET UNITY_SRC=%HOST_DIR%\gen_win32.cpp

:: output
SET ENGINE_EXE=%BUILD_DIR%\mat_gen.exe

:: TODO(Dustin): Debug vs Release mode
:: Linking and Compiling info
SET CFLAGS=/Zi /std:c++17 /EHsc

:: Format: /LIBPATH:library
SET EXT_LIB=%HOST_DIR%\spriv\binaries\spirv-cross-cored.lib %HOST_DIR%\spriv\binaries\spirv-cross-glsld.lib
SET GBL_LIB=%EXT_LIB% ucrtd.lib user32.lib Gdi32.lib

SET INC=/I%EXT_DIR%

IF NOT EXIST build\data\models\models\ (
    1>NUL MKDIR build\data\models\models\
)

IF NOT EXIST build\data\models\binaries\ (
    1>NUL MKDIR build\data\models\binaries\
)

IF NOT EXIST build\data\materials\ (
    1>NUL MKDIR build\data\materials\
)

pushd %BUILD_DIR%\%GAME%
	
	echo Building platform...
	echo cl /MDd %CFLAGS% %INC% %UNITY_SRC% /Fe%ENGINE_EXE% /link %GBL_LIB%
	cl /MDd %CFLAGS% %INC% %UNITY_SRC% /Fe%ENGINE_EXE% /link %GBL_LIB%

popd

