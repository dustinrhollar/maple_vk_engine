@echo off
setlocal EnableDelayedExpansion

:: External library directories
SET VK_COMPILER="C:\VulkanSDK\1.2.135.0\Bin"

:: Project directory
SET HOST_DIR=%~dp0
set HOST_DIR=%HOST_DIR:~0,-1%

:: Flags for Platform
SET MP_CFLAGS=-std=c99 -g -D_DEBUG -Wno-microsoft-include
SET MP_INC=
SET MP_LIB=-llibcpmtd.lib -luser32.lib -lGdi32.lib -lwinmm.lib
SET MP_INPUT=%HOST_DIR%\platform\engine_unity.c
SET MP_OUTPUT=maple.exe
SET MP_DEFS=-DVK_NO_PROTOTYPES

:: Flags for Graphics
SET VK_CFLAGS=/Zi /MTd /std:c++17 -nologo /EHsc /D_DEBUG
SET VK_INC=
SET VK_LIB=%LIB_PATH% libcpmtd.lib user32.lib Gdi32.lib winmm.lib
SET VK_INPUT=%HOST_DIR%\graphics\graphics_unity.cpp
SET VK_OUTPUT=/Femaple_vk.dll
SET VK_EXPORTS=
SET VK_DEFS=/DVK_USE_PLATFORM_WIN32_KHR /DVK_NO_PROTOTYPES /DGRAPHICS_DLL_EXPORT

:: Flags for the Game
SET GM_CFLAGS=-std=c99 -g -D_DEBUG -Wno-microsoft-include
SET GM_INC=
SET GM_LIB=
SET GM_INPUT=%HOST_DIR%\game\game_unity.c
SET GM_OUTPUT=maple_game.dll
SET GM_EXPORTS=
SET GM_DEFS=-DGAME_DLL_EXPORT

IF NOT EXIST build\data\terrain\ (
    1>NUL MKDIR build\data\terrain\
)

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

IF NOT EXIST build\data\textures\ (
    1>NUL MKDIR build\data\textures\
)

:: TODO(Dustin): Get unique names for the games pdb file so that we can hot reload in the debugger
IF "%1" == "gm" (
	:: /PDB:maple_game_%time%.pdb
    pushd build\
		echo clang %GM_CFLAGS% %GM_DEFS% %GM_INC% %GM_INPUT% -shared -o%GM_OUTPUT% %GM_LIB%
		clang %GM_CFLAGS% %GM_DEFS% %GM_INC% %GM_INPUT% -shared -o%GM_OUTPUT% %GM_LIB%
		popd
    EXIT /B %ERRORLEVEL%
)

IF "%1" == "vk" (
    pushd build\
		echo Building maple graphics...
		echo cl /DVK_USE_PLATFORM_WIN32_KHR %VK_CFLAGS% %VK_INC% %VK_INPUT% /LD %VK_OUTPUT% /link %VK_LIB% %VK_EXPORTS%
        cl %VK_DEFS% %VK_CFLAGS% %VK_INC% %VK_INPUT% /LD %VK_OUTPUT% /link %VK_LIB% %VK_EXPORTS%
  	popd
    EXIT /B %ERRORLEVEL%
)

IF "%1" == "mp" (
    pushd build\
        echo Building maple engine...
        echo clang %MP_CFLAGS% %MP_INC% %MP_INPUT% -o%MP_OUTPUT% %MP_LIB%
	    clang %MP_CFLAGS% %MP_DEFS% %MP_INC% %MP_INPUT% -o%MP_OUTPUT% %MP_LIB%
	popd
    EXIT /B %ERRORLEVEL%
)
