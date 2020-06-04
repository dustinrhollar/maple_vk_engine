@echo off
setlocal EnableDelayedExpansion

:: External library directories
SET VK_COMPILER="C:\VulkanSDK\1.2.135.0\Bin"


:: Project directories
SET HOST_DIR=%~dp0
set HOST_DIR=%HOST_DIR:~0,-1%
SET INC_DIR=
SET EXT_DIR=%HOST_DIR%\ext
SET ENGINE_DIR=%HOST_DIR%\platform
SET SRC_DIR=%HOST_DIR%\example
SET BUILD_DIR=%HOST_DIR%\build

:: unity Build files
SET UNITY_SRC=%HOST_DIR%\unity.cpp

:: output
SET ENGINE_EXE=%BUILD_DIR%\maple.exe

:: TODO(Dustin): Debug vs Release mode
:: Linking and Compiling info
SET CFLAGS=/Zi /std:c++17 /EHsc

:: Format: /LIBPATH:library
SET LIB_PATH=/LIBPATH:%VK_COMPILER%
SET EXT_LIB=
SET GBL_LIB=%LIB_PATH% %EXT_LIB% libcpmtd.lib user32.lib Gdi32.lib winmm.lib

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

IF NOT EXIST build\data\mat_refl\ (
    1>NUL MKDIR build\data\mat_refl\
)

pushd %BUILD_DIR%\%GAME%
	
	echo Building platform...
	echo cl /MTd -DVK_USE_PLATFORM_WIN32_KHR %CFLAGS% %INC% %UNITY_SRC% /Fe%ENGINE_EXE% /link %GBL_LIB%
	cl /MTd -DVK_USE_PLATFORM_WIN32_KHR %CFLAGS% %INC% %UNITY_SRC% /Fe%ENGINE_EXE% /link %GBL_LIB%

popd

