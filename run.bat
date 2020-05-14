@echo off
setlocal EnableDelayedExpansion

SET HOST_DIR=%~dp0
SET HOST_DIR=%HOST_DIR:~0,-1%

SET EXE_NAME=maple.exe

:: Build Commands
SET NONE=""
SET BUILD=build
SET CLEAN=clean
SET DEBUG=debug
SET EXT=build_ext
SET RSRC=rsrc
SET SHAD=shad

GOTO :MAIN
EXIT /B %ERRORLEVEL%

:Build
	call build.bat
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

	pushd %HOST_DIR%\build
	    IF NOT EXIST data (
        	1>NUL md data
    	)

		xcopy %HOST_DIR%\data data\ /i /d /y /s
	popd
	EXIT /B 0

:BuildShaders
	pushd build\data\shaders
        call build.bat
    popd
    EXIT /B 0

:MAIN
    IF NOT EXIST build (
        1>NUL md build
    )

    IF "%1" == %NONE% (
        pushd build\
            .\%EXE_NAME%
        popd
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