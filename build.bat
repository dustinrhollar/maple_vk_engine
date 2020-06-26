@echo off
setlocal EnableDelayedExpansion

:: External library directories
SET VK_COMPILER="C:\VulkanSDK\1.2.135.0\Bin"
SET D3D_COMPILER="C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Lib\x64"

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
SET LIB_PATH=/LIBPATH:%D3D_COMPILER%
SET EXT_LIB=
SET GBL_LIB=%LIB_PATH% libcpmtd.lib user32.lib Gdi32.lib winmm.lib d3d11.lib d3dx11.lib

SET INC=/I"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include"

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
        echo Building game...
		cl /MT -nologo /Zi /EHsc /I%HOST_DIR%\engine %HOST_DIR%\example\example_unity.cpp /LD /Feexample.dll /link /EXPORT:GameStageEntry
	popd
    EXIT /B %ERRORLEVEL%
)

IF "%1" == "mp" (
    pushd build\
        echo Building maple engine...
        cl /MT -nologo /Zi /EHsc %INC% %HOST_DIR%\engine\engine_unity.cpp /Femaple.exe /link %GBL_LIB%
    popd
    EXIT /B %ERRORLEVEL%
)
