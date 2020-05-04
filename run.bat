@echo off
setlocal EnableDelayedExpansion

SET EXE_NAME=splicer.exe

:: Build Commands
SET NONE=""
SET CONFIGURE=conf
SET BUILD=build
SET CLEAN=clean
SET DEBUG=debug
SET EXT=build_ext
SET RSRC=rsrc
SET SHAD=shad

GOTO :MAIN
EXIT /B %ERRORLEVEL%

:Configure
    pushd build
        cmake -G "NMake Makefiles" ..
    popd
    EXIT /B 0

:Build
	call build.bat
    pushd build
		::nmake
        ::xcopy splicer\SplicerLib.dll platform /i /d /y
    popd
    EXIT /B 0

:Clean
    del /s /q /f build\*
    EXIT /B 0

:Debug
    pushd build\
		devenv .\%EXE_NAME%
    popd
    EXIT /B 0

:BuildExt
	pushd ext
		call build.bat
	popd
    EXIT /B 0

:CopyRSRC
	xcopy resources build\ /i /d /y /s
	EXIT /B 0

:BuildShaders
	pushd build\shaders
        call build.bat
    popd
    EXIT /B 0

:MAIN
    IF NOT EXIST build (
        1>NUL md build
    )

    IF "%1" == %NONE% (
        pushd build\
            .\%EXE_NAME% > log.txt
        popd
		EXIT /B %ERRORLEVEL%
    )

    IF %1 == %CONFIGURE% (
        CALL :Configure
		EXIT /B %ERRORLEVEL%
    )
    IF %1 == %BUILD% (
        CALL :Build
		EXIT /B %ERRORLEVEL%
    )
    IF %1 == %CLEAN% (
        CALL :Clean
		EXIT /B %ERRORLEVEL%
    )
	IF %1 == %DEBUG% (
        CALL :Debug
		EXIT /B %ERRORLEVEL%
    )
	IF %1 == %EXT% (
        CALL :BuildExt
		EXIT /B %ERRORLEVEL%
    )

	IF %1 == %RSRC% (
        CALL :CopyRSRC
		EXIT /B %ERRORLEVEL%
    )
    
    IF %1 == %SHAD% (
        CALL :BuildShaders
	EXIT /B %ERRORLEVEL%
    )