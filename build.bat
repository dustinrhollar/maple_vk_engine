@echo off
setlocal EnableDelayedExpansion

:: External library directories
SET VK_COMPILER="C:\VulkanSDK\1.2.135.0\Bin"


:: Project directories
SET HOST_DIR=%~dp0
set HOST_DIR=%HOST_DIR:~0,-1%
SET INC_DIR=%HOST_DIR%\inc
SET ENGINE_DIR=%HOST_DIR%\platform
SET SRC_DIR=%HOST_DIR%\example
SET EXT_DIR=%HOST_DIR%\ext
SET BUILD_DIR=%HOST_DIR%\build

SET GLFW_INC=%EXT_DIR%\glfw-3.3.2\include

:: unity Build files
SET PLAT_SRC=%PLAT_DIR%\splicer_win32.cpp
SET GAME_SRC=%SRC_DIR%\unity.cpp

:: output
SET PLAT_EXE=%BUILD_DIR%\splicer.exe
SET GAME_LIB=%BUILD_DIR%\libsplicer.dll
SET PLAT_LIB=glfw3dll.lib

:: Linking and Compiling info
SET CFLAGS=/Zi /std:c++17 /EHsc

:: Format: /LIBPATH:library
SET LIB_PATH=/LIBPATH:%VK_COMPILER%
SET EXT_LIB=
SET GBL_LIB=%LIB_PATH% %EXT_LIB% libcpmtd.lib user32.lib Gdi32.lib winmm.lib

SET INC=/I%INC_DIR% /I%EXT_DIR% /I%PLAT_DIR% /I%SRC_DIR%

pushd %BUILD_DIR%\%GAME%
	
	echo Building game...

	SET EXPORTS=-EXPORT:GameUpdate -EXPORT:GameShutdown -EXPORT:GameInitialize -EXPORT:GameResize

	echo cl %CFLAGS% %INC% /DVK_USE_PLATFORM_WIN32_KHR /I%SRC_DIR% %GAME_SRC% /LDd /Fe%GAME_LIB% /link %GBL_LIB% %EXPORTS%

	:: Compiles the game
	cl %CFLAGS% %INC% /DVK_USE_PLATFORM_WIN32_KHR /I%SRC_DIR% %GAME_SRC% /LDd /Fe%GAME_LIB% /link %GBL_LIB% %EXPORTS%

	echo Building platform...
	cl /MTd %CFLAGS% %INC% /I%GLFW_INC% %PLAT_SRC% /Fe%PLAT_EXE% /link %PLAT_LIB% %GBL_LIB%

popd

